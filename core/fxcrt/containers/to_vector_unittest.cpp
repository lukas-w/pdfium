// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/containers/to_vector.h"

#include <stdint.h>

#include <array>
#include <vector>

#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ToVectorTest, FromSpan) {
  static constexpr uint8_t kData[] = {1, 2, 3, 4};
  pdfium::span<const uint8_t> span(kData);
  std::vector<uint8_t> vec = pdfium::ToVector(span);
  ASSERT_EQ(4u, vec.size());
  EXPECT_EQ(1, vec[0]);
  EXPECT_EQ(2, vec[1]);
  EXPECT_EQ(3, vec[2]);
  EXPECT_EQ(4, vec[3]);
}

TEST(ToVectorTest, FromEmptySpan) {
  pdfium::span<const int> span;
  std::vector<int> vec = pdfium::ToVector(span);
  EXPECT_TRUE(vec.empty());
}

TEST(ToVectorTest, FromArray) {
  std::array<int, 3> arr = {10, 20, 30};
  std::vector<int> vec = pdfium::ToVector(arr);
  ASSERT_EQ(3u, vec.size());
  EXPECT_EQ(10, vec[0]);
  EXPECT_EQ(20, vec[1]);
  EXPECT_EQ(30, vec[2]);
}

TEST(ToVectorTest, WithExplicitType) {
  static constexpr uint16_t kData[] = {10, 20, 30};
  pdfium::span<const uint16_t> span(kData);
  std::vector<uint32_t> vec = pdfium::ToVector<uint32_t>(span);
  ASSERT_EQ(3u, vec.size());
  EXPECT_EQ(10u, vec[0]);
  EXPECT_EQ(20u, vec[1]);
  EXPECT_EQ(30u, vec[2]);
}
