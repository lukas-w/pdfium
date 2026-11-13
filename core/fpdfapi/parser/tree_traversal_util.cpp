// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/tree_traversal_util.h"

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/raw_span.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"

namespace pdfium {

namespace {

// Traverses an `AdjacencyMap` using iterative DFS, ignoring self-references and
// ancestor back-edges (cycles).
class DfsTraverser {
 public:
  DfsTraverser(pdfium::span<const NodeId> roots,
               const AdjacencyMap* adjacency_map)
      : roots_(roots), adjacency_map_(adjacency_map) {}

  RefcountMap Calculate() {
    for (NodeId root : roots_) {
      ++ref_counts_[root];
    }
    for (NodeId root : roots_) {
      Traverse(root);
    }
    for (const auto& it : *adjacency_map_) {
      Traverse(it.first);
    }
    return std::move(ref_counts_);
  }

 private:
  struct Frame {
    NodeId node;
    size_t child_index = 0;
  };

  bool IsOnStack(NodeId node) const {
    return Contains(fast_lookup_for_stack_, node);
  }
  bool IsFullyVisited(NodeId node) const {
    return Contains(fully_visited_, node);
  }

  void Push(NodeId node) {
    CHECK(!IsFullyVisited(node));
    CHECK(fast_lookup_for_stack_.insert(node).second);
    stack_.push_back({node, 0});
  }

  void Pop() {
    CHECK(!stack_.empty());
    const NodeId node = stack_.back().node;
    stack_.pop_back();
    CHECK_EQ(fast_lookup_for_stack_.erase(node), 1u);
    fully_visited_.insert(node);
  }

  void Traverse(NodeId root) {
    if (IsFullyVisited(root)) {
      return;
    }
    Push(root);

    while (!stack_.empty()) {
      const NodeId current_node = stack_.back().node;
      const size_t current_child_index = stack_.back().child_index;
      auto it = adjacency_map_->find(current_node);
      if (it == adjacency_map_->end() ||
          current_child_index >= it->second.size()) {
        Pop();
        continue;
      }

      const NodeId child = it->second[current_child_index];
      stack_.back().child_index++;
      if (child == current_node || IsOnStack(child)) {
        // Self-reference or back-edge (cycle).
        continue;
      }

      ++ref_counts_[child];
      if (IsFullyVisited(child)) {
        // Cross-edge or forward-edge in DAG: already explored.
        continue;
      }

      // Tree-edge: advance into unvisited node.
      Push(child);
    }
  }

  pdfium::raw_span<const NodeId> const roots_;
  UnownedPtr<const AdjacencyMap> const adjacency_map_;
  std::vector<Frame> stack_;
  std::set<NodeId> fast_lookup_for_stack_;
  std::set<NodeId> fully_visited_;
  RefcountMap ref_counts_;
};

}  // namespace

RefcountMap CalculateReferenceCounts(pdfium::span<const NodeId> roots,
                                     const AdjacencyMap& adjacency_map) {
  return DfsTraverser(roots, &adjacency_map).Calculate();
}

}  // namespace pdfium
