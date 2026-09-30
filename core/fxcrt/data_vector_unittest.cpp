// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/data_vector.h"

#include <stdint.h>

#include <array>

#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcrt {

TEST(DataVectorTest, ToDataVectorFromSpan) {
  static constexpr uint8_t kData[] = {1, 2, 3, 4};
  pdfium::span<const uint8_t> span(kData);
  DataVector<uint8_t> vec = ToDataVector(span);
  ASSERT_EQ(4u, vec.size());
  EXPECT_EQ(1, vec[0]);
  EXPECT_EQ(2, vec[1]);
  EXPECT_EQ(3, vec[2]);
  EXPECT_EQ(4, vec[3]);
}

TEST(DataVectorTest, ToDataVectorFromEmptySpan) {
  pdfium::span<const int> span;
  DataVector<int> vec = ToDataVector(span);
  EXPECT_TRUE(vec.empty());
}

TEST(DataVectorTest, ToDataVectorFromArray) {
  std::array<int, 3> arr = {10, 20, 30};
  DataVector<int> vec = ToDataVector(arr);
  ASSERT_EQ(3u, vec.size());
  EXPECT_EQ(10, vec[0]);
  EXPECT_EQ(20, vec[1]);
  EXPECT_EQ(30, vec[2]);
}

TEST(DataVectorTest, ToDataVectorWithExplicitType) {
  static constexpr uint16_t kData[] = {10, 20, 30};
  pdfium::span<const uint16_t> span(kData);
  DataVector<uint32_t> vec = ToDataVector<uint32_t>(span);
  ASSERT_EQ(3u, vec.size());
  EXPECT_EQ(10u, vec[0]);
  EXPECT_EQ(20u, vec[1]);
  EXPECT_EQ(30u, vec[2]);
}

}  // namespace fxcrt
