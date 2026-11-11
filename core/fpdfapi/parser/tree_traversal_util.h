// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_PARSER_TREE_TRAVERSAL_UTIL_H_
#define CORE_FPDFAPI_PARSER_TREE_TRAVERSAL_UTIL_H_

#include <stdint.h>

#include <map>

#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/span.h"

namespace pdfium {

using NodeId = uint32_t;
using AdjacencyMap = std::map<NodeId, DataVector<NodeId>>;
using RefcountMap = std::map<NodeId, int>;

// Given roots and an adjacency map of a directed graph, calculates reference
// counts for all reachable nodes while ignoring self-references and back-edges
// (cycles). Also traverses any remaining disconnected components in
// `adjacency_map`.
RefcountMap CalculateReferenceCounts(pdfium::span<const NodeId> roots,
                                     const AdjacencyMap& adjacency_map);

}  // namespace pdfium

#endif  // CORE_FPDFAPI_PARSER_TREE_TRAVERSAL_UTIL_H_
