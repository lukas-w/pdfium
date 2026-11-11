// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/tree_traversal_util.h"

#include <stdint.h>

#include <map>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;
using testing::Pair;

namespace pdfium {

TEST(TreeTraversalUtilTest, EmptyGraph) {
  AdjacencyMap adj;
  std::map<NodeId, int> counts = CalculateReferenceCounts({}, adj);
  EXPECT_TRUE(counts.empty());
}

TEST(TreeTraversalUtilTest, SingleNodeRoot) {
  AdjacencyMap adj;
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_THAT(counts, ElementsAre(Pair(1, 1)));
}

TEST(TreeTraversalUtilTest, SimpleTree) {
  // 1 -> 2
  // 1 -> 3
  AdjacencyMap adj = {{1, {2, 3}}};
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_THAT(counts, ElementsAre(Pair(1, 1), Pair(2, 1), Pair(3, 1)));
}

TEST(TreeTraversalUtilTest, DiamondDAGCrossEdge) {
  // 1 -> 2, 3
  // 2 -> 4
  // 3 -> 4
  AdjacencyMap adj = {{1, {2, 3}}, {2, {4}}, {3, {4}}};
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_THAT(counts,
              ElementsAre(Pair(1, 1), Pair(2, 1), Pair(3, 1), Pair(4, 2)));
}

TEST(TreeTraversalUtilTest, SelfLoopIgnored) {
  // 1 -> 2 -> 2
  AdjacencyMap adj = {{1, {2}}, {2, {2}}};
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_THAT(counts, ElementsAre(Pair(1, 1), Pair(2, 1)));
}

TEST(TreeTraversalUtilTest, ForwardEdge) {
  // 1 -> 2 -> 3
  // 1 -> 3
  AdjacencyMap adj = {{1, {2, 3}}, {2, {3}}};
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_THAT(counts, ElementsAre(Pair(1, 1), Pair(2, 1), Pair(3, 2)));
}

TEST(TreeTraversalUtilTest, DeepLinearChainNoStackOverflow) {
  // 1 -> 2 -> 3 -> ... -> 10000
  AdjacencyMap adj;
  for (uint32_t i = 1; i < 10000; ++i) {
    adj[i].push_back(i + 1);
  }
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_EQ(10000u, counts.size());
  EXPECT_EQ(1, counts.at(1));
  EXPECT_EQ(1, counts.at(5000));
  EXPECT_EQ(1, counts.at(10000));
}

TEST(TreeTraversalUtilTest, DisconnectedComponent) {
  // Root 1 -> 2
  // Disconnected 3 -> 4
  AdjacencyMap adj = {{1, {2}}, {3, {4}}};
  std::map<NodeId, int> counts = CalculateReferenceCounts({1}, adj);
  EXPECT_THAT(counts, ElementsAre(Pair(1, 1), Pair(2, 1), Pair(4, 1)));
}

}  // namespace pdfium
