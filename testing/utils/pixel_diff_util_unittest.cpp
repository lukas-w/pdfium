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
  EXPECT_DOUBLE_EQ(0.0, CalculateMaxWindowMSE(img, 8, img, 8, 8, 8, 8));
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEUniformDelta) {
  // Uniform delta of 1 on red channel across entire image.
  std::vector<uint32_t> img1(64, 0x00000000);
  std::vector<uint32_t> img2(64, 0x00000001);
  // Squared error = 1 on 1 channel per pixel -> local MSE = 1 / 3 = 0.3333...
  EXPECT_NEAR(1.0 / 3.0, CalculateMaxWindowMSE(img1, 8, img2, 8, 8, 8, 4),
              1e-6);
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
                   CalculateMaxWindowMSE(baseline, 16, actual, 16, 16, 16, 8));
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
  EXPECT_DOUBLE_EQ(12.5,
                   CalculateMaxWindowMSE(baseline, 8, actual, 8, 8, 8, 8));
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEWithStrides) {
  // 8x8 image within a buffer with stride = 12.
  constexpr int kWidth = 8;
  constexpr int kHeight = 8;
  constexpr size_t kStride = 12;
  std::vector<uint32_t> baseline(kStride * kHeight, 0x00000000);
  std::vector<uint32_t> actual = baseline;
  // Uniform delta of 1 on red channel inside the 8x8 region.
  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      actual[y * kStride + x] = 0x00000001;
    }
  }
  // Padding pixels outside 8x8 should not affect calculation.
  for (int y = 0; y < kHeight; ++y) {
    for (size_t x = kWidth; x < kStride; ++x) {
      actual[y * kStride + x] = 0x00ffffff;
    }
  }
  EXPECT_NEAR(1.0 / 3.0,
              CalculateMaxWindowMSE(baseline, kStride, actual, kStride, kWidth,
                                    kHeight, 4),
              1e-6);
}

TEST(PixelDiffUtilTest, CalculateMaxWindowMSEInvalidInputs) {
  std::vector<uint32_t> img(16, 0);
  EXPECT_DOUBLE_EQ(0.0, CalculateMaxWindowMSE(img, 0, img, 0, 0, 0, 8));
  EXPECT_DOUBLE_EQ(0.0, CalculateMaxWindowMSE(img, 4, img, 4, 4, 4, 0));
}

TEST(PixelDiffUtilTest, CalculatePixelsDifferentExactMatch) {
  std::vector<uint32_t> img(64, 0xffffffff);
  EXPECT_EQ(0,
            CalculatePixelsDifferent(img, 8, img, 8, 8, 8, kExactDiffOptions));
}

TEST(PixelDiffUtilTest, CalculatePixelsDifferentTolerance) {
  std::vector<uint32_t> baseline(64, 0x00000000);
  std::vector<uint32_t> actual = baseline;
  actual[0] = 0x00020202;  // delta 2 on all channels
  // Exact match fails.
  EXPECT_EQ(1, CalculatePixelsDifferent(baseline, 8, actual, 8, 8, 8,
                                        kExactDiffOptions));
  // Tolerating delta 2 passes.
  EXPECT_EQ(0, CalculatePixelsDifferent(
                   baseline, 8, actual, 8, 8, 8,
                   DiffOptions{.max_pixel_per_channel_delta = 2}));
  // Tolerating delta 2 but strict MSE (0.0001) fails.
  EXPECT_EQ(1, CalculatePixelsDifferent(
                   baseline, 8, actual, 8, 8, 8,
                   DiffOptions{.max_pixel_per_channel_delta = 2,
                               .max_mean_squared_error = 0.0001}));
  // Window MSE failure triggers.
  EXPECT_EQ(1, CalculatePixelsDifferent(
                   baseline, 8, actual, 8, 8, 8,
                   DiffOptions{.max_pixel_per_channel_delta = 2,
                               .max_mean_squared_error = 1.0,
                               .window_size = 8,
                               .max_window_mean_squared_error = 0.01}));

  // Under exact diff, all 64 pixels differ.
  std::vector<uint32_t> img1(64, 0x00000000);
  std::vector<uint32_t> img2(64, 0x00000001);  // delta 1 on red
  EXPECT_EQ(
      64, CalculatePixelsDifferent(img1, 8, img2, 8, 8, 8, kExactDiffOptions));

  // Under fuzzy diff (tolerates delta <= 3 and MSE <= 0.05).
  // Here MSE = (64 * 1) / (3 * 64) = 0.333 > 0.05, so MSE fails and returns >
  // 0.
  EXPECT_GT(CalculatePixelsDifferent(img1, 8, img2, 8, 8, 8, kFuzzyDiffOptions),
            0);

  // With a relaxed MSE threshold of 0.5, it should report 0 differences.
  DiffOptions relaxed_options = {
      .max_pixel_per_channel_delta = 1,
      .max_mean_squared_error = 0.5,
  };
  EXPECT_EQ(0,
            CalculatePixelsDifferent(img1, 8, img2, 8, 8, 8, relaxed_options));
}

TEST(PixelDiffUtilTest, DiffOptionsAggregateAndDefaults) {
  static_assert(std::is_aggregate_v<DiffOptions>);

  constexpr DiffOptions default_options;
  EXPECT_EQ(0, default_options.max_pixel_per_channel_delta);
  EXPECT_DOUBLE_EQ(0.0, default_options.max_mean_squared_error);
  EXPECT_EQ(0, default_options.window_size);
  EXPECT_DOUBLE_EQ(0.0, default_options.max_window_mean_squared_error);

  EXPECT_EQ(0, kExactDiffOptions.max_pixel_per_channel_delta);
  EXPECT_DOUBLE_EQ(0.0, kExactDiffOptions.max_mean_squared_error);
  EXPECT_EQ(0, kExactDiffOptions.window_size);
  EXPECT_DOUBLE_EQ(0.0, kExactDiffOptions.max_window_mean_squared_error);

  EXPECT_EQ(kMaxFuzzyPixelDelta, kFuzzyDiffOptions.max_pixel_per_channel_delta);
  EXPECT_DOUBLE_EQ(kMaxFuzzyMeanSquaredError,
                   kFuzzyDiffOptions.max_mean_squared_error);
  EXPECT_EQ(kMaxFuzzyWindowSize, kFuzzyDiffOptions.window_size);
  EXPECT_DOUBLE_EQ(kMaxFuzzyWindowMeanSquaredError,
                   kFuzzyDiffOptions.max_window_mean_squared_error);

  // Verification with options struct.
  std::vector<uint32_t> baseline(64, 0x00000000);
  std::vector<uint32_t> actual = baseline;
  actual[0] = 0x00000002;
  EXPECT_EQ(1, CalculatePixelsDifferent(baseline, 8, actual, 8, 8, 8,
                                        kExactDiffOptions));
  EXPECT_EQ(0, CalculatePixelsDifferent(baseline, 8, actual, 8, 8, 8,
                                        kFuzzyDiffOptions));
}
