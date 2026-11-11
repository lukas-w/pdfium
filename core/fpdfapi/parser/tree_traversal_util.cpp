// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/tree_traversal_util.h"

#include <stdint.h>

#include <map>
#include <queue>
#include <set>
#include <vector>

#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/span.h"

namespace pdfium {

std::map<NodeId, int> CalculateReferenceCounts(pdfium::span<const NodeId> roots,
                                               const AdjacencyMap& adj) {
  std::map<NodeId, int> ref_counts;
  for (NodeId root : roots) {
    ++ref_counts[root];
  }

  // Tracks nodes that have previously acted as the source of a reference edge.
  std::set<NodeId> seen_ref_objects;
  std::set<NodeId> queued_nodes;
  std::queue<NodeId> queue;

  auto enqueue = [&](NodeId node) {
    const bool inserted = queued_nodes.insert(node).second;
    if (inserted) {
      queue.push(node);
    }
    return inserted;
  };

  auto drain_queue = [&]() {
    while (!queue.empty()) {
      NodeId u = queue.front();
      queue.pop();

      auto it = adj.find(u);
      if (it == adj.end()) {
        continue;
      }

      for (NodeId v : it->second) {
        // Make sure this is not a self-reference.
        if (u == v) {
          continue;
        }

        // Make sure this is not a circular reference.
        if (pdfium::Contains(seen_ref_objects, u) &&
            pdfium::Contains(seen_ref_objects, v)) {
          continue;
        }

        ++ref_counts[v];
        seen_ref_objects.insert(u);
        enqueue(v);
      }
    }
  };

  for (NodeId root : roots) {
    enqueue(root);
  }
  drain_queue();

  for (const auto& it : adj) {
    if (enqueue(it.first)) {
      drain_queue();
    }
  }

  return ref_counts;
}

}  // namespace pdfium
