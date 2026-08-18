// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/pixel_diff_util.h"

#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"

TEST(PixelDiffUtilTest, MaxPixelPerChannelDeltaExactMatch) {
  EXPECT_EQ(0, MaxPixelPerChannelDelta(0x00000000, 0x00000000));
  EXPECT_EQ(0, MaxPixelPerChannelDelta(0xffffffff, 0xffffffff));
  EXPECT_EQ(0, MaxPixelPerChannelDelta(0x12345678, 0x12345678));
}

TEST(PixelDiffUtilTest, MaxPixelPerChannelDeltaSingleChannel) {
  EXPECT_EQ(5, MaxPixelPerChannelDelta(0x00000005, 0x00000000));
  EXPECT_EQ(10, MaxPixelPerChannelDelta(0x00000a00, 0x00000000));
  EXPECT_EQ(15, MaxPixelPerChannelDelta(0x000f0000, 0x00000000));
  EXPECT_EQ(20, MaxPixelPerChannelDelta(0x14000000, 0x00000000));
  EXPECT_EQ(255, MaxPixelPerChannelDelta(0x000000ff, 0x00000000));
}

TEST(PixelDiffUtilTest, MaxPixelPerChannelDeltaMultipleChannels) {
  EXPECT_EQ(8, MaxPixelPerChannelDelta(0x01020304, 0x09020304));
  EXPECT_EQ(7, MaxPixelPerChannelDelta(0x01020304, 0x01090304));
  EXPECT_EQ(6, MaxPixelPerChannelDelta(0x01020304, 0x01020904));
  EXPECT_EQ(5, MaxPixelPerChannelDelta(0x01020304, 0x01020309));
}

TEST(PixelDiffUtilTest, PixelSquaredError) {
  EXPECT_EQ(0u, PixelSquaredError(0x00000000, 0x00000000));
  EXPECT_EQ(0u, PixelSquaredError(0xffffffff, 0xffffffff));

  // Differences in color channels (bytes 0, 1, 2).
  EXPECT_EQ(4u, PixelSquaredError(0x00000002, 0x00000000));
  EXPECT_EQ(9u, PixelSquaredError(0x00000300, 0x00000000));
  EXPECT_EQ(16u, PixelSquaredError(0x00040000, 0x00000000));
  EXPECT_EQ(29u, PixelSquaredError(0x00040302, 0x00000000));

  // Alpha (byte 3) is ignored for color MSE calculation.
  EXPECT_EQ(0u, PixelSquaredError(0xff000000, 0x00000000));
}

TEST(PixelDiffUtilTest, FuzzyThresholds) {
  EXPECT_EQ(3, kMaxFuzzyPixelDelta);
  EXPECT_DOUBLE_EQ(0.05, kMaxFuzzyMeanSquaredError);
}
