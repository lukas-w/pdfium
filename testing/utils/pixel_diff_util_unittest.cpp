// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/pixel_diff_util.h"

#include <stdint.h>

#include <vector>

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
  EXPECT_EQ(8, kMaxFuzzyWindowSize);
  EXPECT_DOUBLE_EQ(15.0, kMaxFuzzyWindowMeanSquaredError);
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEExactMatch) {
  std::vector<uint32_t> img(64, 0xffffffff);
  EXPECT_DOUBLE_EQ(0.0, CalculateMaxWindowMSE(img, img, 8, 8, 8));
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEUniformDelta) {
  // Uniform delta of 1 on red channel across entire image.
  std::vector<uint32_t> img1(64, 0x00000000);
  std::vector<uint32_t> img2(64, 0x00000001);
  // Squared error = 1 on 1 channel per pixel -> local MSE = 1 / 3 = 0.3333...
  EXPECT_NEAR(1.0 / 3.0, CalculateMaxWindowMSE(img1, img2, 8, 8, 4), 1e-6);
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSELocalizedDefect) {
  // 16x16 white image with a 4x4 black box defect at (4, 4).
  std::vector<uint32_t> baseline(16 * 16, 0x00ffffff);
  std::vector<uint32_t> actual = baseline;
  for (int y = 4; y < 8; ++y) {
    for (int x = 4; x < 8; ++x) {
      actual[y * 16 + x] = 0x00000000;
    }
  }

  // Window size = 8.
  // The window from (4, 4) to (12, 12) captures all 16 defect pixels.
  // Defect error = 16 pixels * (3 * 255^2) = 3121200.
  // Window MSE = 3121200 / (3 * 64) = 16256.25.
  EXPECT_DOUBLE_EQ(16256.25,
                   CalculateMaxWindowMSE(baseline, actual, 16, 16, 8));
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEChromaEdge) {
  // 8x8 image with an 8-pixel line of delta = 10 on all 3 channels.
  std::vector<uint32_t> baseline(8 * 8, 0x00000000);
  std::vector<uint32_t> actual = baseline;
  for (int x = 0; x < 8; ++x) {
    actual[x] = 0x000a0a0a;  // r=10, g=10, b=10
  }

  // 8 pixels * 3 channels * 10^2 = 2400.
  // Window MSE = 2400 / (3 * 64) = 12.5.
  EXPECT_DOUBLE_EQ(12.5, CalculateMaxWindowMSE(baseline, actual, 8, 8, 8));
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEInvalidInputs) {
  std::vector<uint32_t> img(16, 0);
  EXPECT_DOUBLE_EQ(0.0, CalculateMaxWindowMSE(img, img, 0, 0, 8));
  EXPECT_DOUBLE_EQ(0.0, CalculateMaxWindowMSE(img, img, 4, 4, 0));
}
