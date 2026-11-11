// Copyright 2023 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/object_tree_traversal_util.h"

#include <stdint.h>

#include <map>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/tree_traversal_util.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/unowned_ptr.h"

namespace {

// Traverses the PDF object hierarchy in a document to extract an `AdjacencyMap`
// and root node IDs.
class PdfReferenceGraphBuilder {
 public:
  explicit PdfReferenceGraphBuilder(const CPDF_Document* document)
      : document_(document) {
    const CPDF_Parser* parser = document_->GetParser();
    const CPDF_Dictionary* trailer = parser ? parser->GetTrailer() : nullptr;
    if (trailer) {
      EnqueueRoot(pdfium::WrapRetain(trailer),
                  parser->GetTrailerObjectNumber());
    }
  }

  PdfReferenceGraph Build() {
    DrainObjectQueue();

    // Also traverse the catalog if it was not reached from the trailer
    // (e.g. if the trailer's `/Root` was missing, null, or diverged).
    const CPDF_Dictionary* catalog = document_->GetRoot();
    if (catalog && !pdfium::Contains(seen_objects_, catalog)) {
      EnqueueRoot(pdfium::WrapRetain(catalog), catalog->GetObjNum());
      DrainObjectQueue();
    }

    return {std::move(roots_), std::move(adj_)};
  }

 private:
  void EnqueueRoot(RetainPtr<const CPDF_Dictionary> root,
                   uint32_t root_object_number) {
    if (!root || !seen_objects_.insert(root.Get()).second) {
      return;
    }
    if (root_object_number) {
      roots_.push_back(root_object_number);
      object_number_map_[root.Get()] = root_object_number;
    }
    object_queue_.push(std::move(root));
  }

  void DrainObjectQueue() {
    while (!object_queue_.empty()) {
      RetainPtr<const CPDF_Object> current_object = object_queue_.front();
      object_queue_.pop();

      switch (current_object->GetType()) {
        case CPDF_Object::kArray: {
          CPDF_ArrayLocker locker(current_object->AsArray());
          for (const auto& it : locker) {
            PushNewObject(current_object, it);
          }
          break;
        }
        case CPDF_Object::kDictionary: {
          CPDF_DictionaryLocker locker(current_object->AsDictionary());
          for (const auto& it : locker) {
            PushNewObject(current_object, it.second);
          }
          break;
        }
        case CPDF_Object::kReference: {
          const CPDF_Reference* ref_object = current_object->AsReference();
          const uint32_t ref_object_number = GetObjectNumber(ref_object);
          const uint32_t referenced_object_number = ref_object->GetRefObjNum();

          RetainPtr<const CPDF_Object> referenced_object;
          if (ref_object->HasIndirectObjectHolder()) {
            // Calling GetIndirectObject() does not work for normal references.
            referenced_object = ref_object->GetDirect();
          } else {
            // Calling GetDirect() does not work for references from trailers.
            referenced_object =
                document_->GetIndirectObject(referenced_object_number);
          }
          // Unlike the other object types, CPDF_Reference can point at nullptr.
          if (referenced_object) {
            CHECK(referenced_object_number);
            if (ref_object_number) {
              if (referenced_object_number != ref_object_number) {
                adj_[ref_object_number].push_back(referenced_object_number);
              }
            } else {
              roots_.push_back(referenced_object_number);
            }
            PushNewObject(ref_object, referenced_object);
          }
          break;
        }
        case CPDF_Object::kStream: {
          RetainPtr<const CPDF_Dictionary> dict =
              current_object->AsStream()->GetDict();
          CHECK(dict->IsInline());  // i.e. No object number.
          CPDF_DictionaryLocker locker(dict);
          for (const auto& it : locker) {
            PushNewObject(current_object, it.second);
          }
          break;
        }
        default: {
          break;
        }
      }
    }
  }

  void PushNewObject(const CPDF_Object* parent_object,
                     RetainPtr<const CPDF_Object> child_object) {
    CHECK(parent_object);
    CHECK(child_object);
    const bool inserted = seen_objects_.insert(child_object).second;
    if (!inserted) {
      return;
    }
    const uint32_t child_object_number = child_object->GetObjNum();
    if (child_object_number) {
      object_number_map_[child_object] = child_object_number;
    } else {
      // This search can fail for inlined trailers.
      auto it = object_number_map_.find(parent_object);
      if (it != object_number_map_.end()) {
        object_number_map_[child_object] = it->second;
      }
    }
    object_queue_.push(std::move(child_object));
  }

  // Returns 0 if not found.
  uint32_t GetObjectNumber(const CPDF_Object* object) const {
    auto it = object_number_map_.find(object);
    return it != object_number_map_.end() ? it->second : 0;
  }

  UnownedPtr<const CPDF_Document> const document_;
  std::vector<pdfium::NodeId> roots_;
  pdfium::AdjacencyMap adj_;

  // Queue of objects to traverse.
  // - Pointers in the queue are non-null.
  // - The same pointer never enters the queue twice.
  std::queue<RetainPtr<const CPDF_Object>> object_queue_;

  // Map of objects to "top-level" object numbers. For inline objects, this is
  // the ancestor object with an object number. The keys are non-null and the
  // values are never 0.
  // This is used to prevent self-references, as a single PDF object, with
  // inlined objects, is represented by multiple CPDF_Objects.
  std::map<const CPDF_Object*, uint32_t> object_number_map_;

  // Tracks traversed objects to prevent duplicates from getting into
  // `object_queue_` and `object_number_map_`.
  std::set<const CPDF_Object*> seen_objects_;
};

}  // namespace

PdfReferenceGraph BuildPdfReferenceGraphForTesting(
    const CPDF_Document* document) {
  return PdfReferenceGraphBuilder(document).Build();
}

std::set<uint32_t> GetObjectsWithReferences(const CPDF_Document* document) {
  PdfReferenceGraph graph = PdfReferenceGraphBuilder(document).Build();
  std::map<pdfium::NodeId, int> counts =
      pdfium::CalculateReferenceCounts(graph.roots, graph.adj);

  std::set<uint32_t> results;
  for (const auto& it : counts) {
    results.insert(it.first);
  }
  return results;
}

std::set<uint32_t> GetObjectsWithMultipleReferences(
    const CPDF_Document* document) {
  PdfReferenceGraph graph = PdfReferenceGraphBuilder(document).Build();
  std::map<pdfium::NodeId, int> counts =
      pdfium::CalculateReferenceCounts(graph.roots, graph.adj);

  std::set<uint32_t> results;
  for (const auto& it : counts) {
    if (it.second > 1) {
      results.insert(it.first);
    }
  }
  return results;
}
