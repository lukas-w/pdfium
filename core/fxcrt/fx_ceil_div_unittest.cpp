// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/fx_ceil_div.h"

#include <stdint.h>

#include <limits>

#include "core/fxcrt/fx_safe_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcrt {

static_assert(CeilDiv(0, 8) == 0);
static_assert(CeilDiv(1, 8) == 1);
static_assert(CeilDiv(8, 8) == 1);
static_assert(CeilDiv(9, 8) == 2);

TEST(CeilDiv, Exact) {
  EXPECT_EQ(0, CeilDiv(0, 4));
  EXPECT_EQ(1, CeilDiv(4, 4));
  EXPECT_EQ(2, CeilDiv(8, 4));
  EXPECT_EQ(25, CeilDiv(100, 4));
}

TEST(CeilDiv, RoundsUp) {
  EXPECT_EQ(1, CeilDiv(1, 4));
  EXPECT_EQ(1, CeilDiv(3, 4));
  EXPECT_EQ(2, CeilDiv(5, 4));
  EXPECT_EQ(2, CeilDiv(7, 4));
}

TEST(CeilDiv, DivisorOfOne) {
  EXPECT_EQ(0, CeilDiv(0, 1));
  EXPECT_EQ(7, CeilDiv(7, 1));
}

TEST(CeilDiv, NegativeDividend) {
  // Rounding towards positive infinity, i.e. towards zero here.
  EXPECT_EQ(0, CeilDiv(-1, 4));
  EXPECT_EQ(0, CeilDiv(-3, 4));
  EXPECT_EQ(-1, CeilDiv(-4, 4));
  EXPECT_EQ(-1, CeilDiv(-7, 4));
  EXPECT_EQ(-2, CeilDiv(-8, 4));
}

TEST(CeilDiv, NoOverflowNearMax) {
  constexpr uint32_t kMax32 = std::numeric_limits<uint32_t>::max();
  EXPECT_EQ(kMax32 / 8 + 1, CeilDiv(kMax32, 8));
  EXPECT_EQ(kMax32, CeilDiv(kMax32, 1));

  constexpr int32_t kMaxSigned32 = std::numeric_limits<int32_t>::max();
  EXPECT_EQ(kMaxSigned32 / 8 + 1, CeilDiv(kMaxSigned32, 8));

  constexpr int32_t kMinSigned32 = std::numeric_limits<int32_t>::min();
  EXPECT_EQ(kMinSigned32 / 8, CeilDiv(kMinSigned32, 8));

  constexpr uint64_t kMax64 = std::numeric_limits<uint64_t>::max();
  EXPECT_EQ(kMax64 / 8 + 1, CeilDiv(kMax64, 8));
}

// The divisor is not deduced, so an untyped literal picks up the dividend's
// type instead of promoting the result to int.
TEST(CeilDiv, NarrowTypes) {
  constexpr uint8_t kMax8 = std::numeric_limits<uint8_t>::max();
  static_assert(std::is_same_v<decltype(CeilDiv(kMax8, 2)), uint8_t>);
  EXPECT_EQ(128, CeilDiv(kMax8, 2));
  EXPECT_EQ(3, CeilDiv(uint8_t{5}, 2));

  constexpr uint16_t kMax16 = std::numeric_limits<uint16_t>::max();
  EXPECT_EQ(32768, CeilDiv(kMax16, 2));
}

TEST(CeilDiv, CheckedType) {
  FX_SAFE_UINT32 value = 9;
  EXPECT_EQ(2u, CeilDiv(value, 8).ValueOrDie());

  FX_SAFE_SIZE_T size = 16;
  EXPECT_EQ(2u, CeilDiv(size, 8).ValueOrDie());
}

TEST(CeilDiv, CheckedTypePropagatesInvalid) {
  FX_SAFE_UINT32 invalid = std::numeric_limits<uint32_t>::max();
  invalid *= 2;
  EXPECT_FALSE(CeilDiv(invalid, 8).IsValid());
}

TEST(CeilDiv, CheckedTypeNoOverflowNearMax) {
  // Dividing cannot overflow, so a valid dividend stays valid no matter how
  // close to the maximum it is.
  constexpr uint32_t kMax32 = std::numeric_limits<uint32_t>::max();
  FX_SAFE_UINT32 near_max = kMax32;
  EXPECT_EQ(kMax32 / 8 + 1, CeilDiv(near_max, 8).ValueOrDie());
}

}  // namespace fxcrt
