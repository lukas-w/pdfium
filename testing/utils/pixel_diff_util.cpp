// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/pixel_diff_util.h"

#include <math.h>
#include <stdint.h>

#include <algorithm>
#include <vector>

namespace {

struct UnpackedPixel {
  explicit UnpackedPixel(uint32_t packed)
      : red(packed & 0xff),
        green((packed >> 8) & 0xff),
        blue((packed >> 16) & 0xff),
        alpha((packed >> 24) & 0xff) {}

  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
};

uint8_t ChannelDelta(uint8_t baseline_channel, uint8_t actual_channel) {
  // No casts are necessary because arithmetic operators implicitly convert
  // `uint8_t` to `int` first. The final delta is always in the range 0 to 255.
  return std::abs(baseline_channel - actual_channel);
}

}  // namespace

uint8_t MaxPixelPerChannelDelta(uint32_t baseline_pixel,
                                uint32_t actual_pixel) {
  UnpackedPixel baseline_unpacked(baseline_pixel);
  UnpackedPixel actual_unpacked(actual_pixel);
  return std::max(
      {ChannelDelta(baseline_unpacked.red, actual_unpacked.red),
       ChannelDelta(baseline_unpacked.green, actual_unpacked.green),
       ChannelDelta(baseline_unpacked.blue, actual_unpacked.blue),
       ChannelDelta(baseline_unpacked.alpha, actual_unpacked.alpha)});
}

uint32_t PixelSquaredError(uint32_t baseline_pixel, uint32_t actual_pixel) {
  UnpackedPixel baseline_unpacked(baseline_pixel);
  UnpackedPixel actual_unpacked(actual_pixel);
  int dr = baseline_unpacked.red - actual_unpacked.red;
  int dg = baseline_unpacked.green - actual_unpacked.green;
  int db = baseline_unpacked.blue - actual_unpacked.blue;
  return static_cast<uint32_t>(dr * dr + dg * dg + db * db);
}

double CalculateMaxWindowMSE(pdfium::span<const uint32_t> baseline,
                             pdfium::span<const uint32_t> actual,
                             int w,
                             int h,
                             int window_size) {
  if (w <= 0 || h <= 0 || window_size <= 0) {
    return 0.0;
  }

  constexpr double kChannelCount = 3.0;
  const size_t total_pixels = static_cast<size_t>(w) * h;
  if (baseline.size() < total_pixels || actual.size() < total_pixels) {
    return 0.0;
  }

  // If image is smaller than window_size in either dimension, evaluate global
  // MSE.
  if (w < window_size || h < window_size) {
    uint64_t total_sq_err = 0;
    for (size_t i = 0; i < total_pixels; ++i) {
      total_sq_err += PixelSquaredError(baseline[i], actual[i]);
    }
    return static_cast<double>(total_sq_err) / (kChannelCount * total_pixels);
  }

  // Compute 2D Summed-Area Table (Integral Image) of squared errors.
  const size_t sat_stride = static_cast<size_t>(w) + 1;
  std::vector<uint64_t> sat(sat_stride * (h + 1), 0);

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const size_t pixel_idx = static_cast<size_t>(y) * w + x;
      const uint64_t sq =
          PixelSquaredError(baseline[pixel_idx], actual[pixel_idx]);
      sat[(y + 1) * sat_stride + (x + 1)] = sq + sat[y * sat_stride + (x + 1)] +
                                            sat[(y + 1) * sat_stride + x] -
                                            sat[y * sat_stride + x];
    }
  }

  // Find maximum local MSE across all overlapping (window_size x window_size)
  // windows.
  double max_win_mse = 0.0;
  const double denom = kChannelCount * window_size * window_size;

  for (int y = 0; y + window_size <= h; ++y) {
    for (int x = 0; x + window_size <= w; ++x) {
      const int x1 = x;
      const int y1 = y;
      const int x2 = x + window_size;
      const int y2 = y + window_size;

      const uint64_t win_sum =
          sat[y2 * sat_stride + x2] - sat[y1 * sat_stride + x2] -
          sat[y2 * sat_stride + x1] + sat[y1 * sat_stride + x1];

      const double win_mse = static_cast<double>(win_sum) / denom;
      max_win_mse = std::max(max_win_mse, win_mse);
    }
  }

  return max_win_mse;
}

int CalculatePixelsDifferent(pdfium::span<const uint32_t> baseline,
                             size_t baseline_stride_pixels,
                             pdfium::span<const uint32_t> actual,
                             size_t actual_stride_pixels,
                             int w,
                             int h,
                             const DiffOptions& options) {
  if (w <= 0 || h <= 0) {
    return 0;
  }

  int pixels_different = 0;
  uint64_t total_squared_error = 0;
  for (int y = 0; y < h; ++y) {
    const size_t baseline_row_offset = y * baseline_stride_pixels;
    const size_t actual_row_offset = y * actual_stride_pixels;
    for (int x = 0; x < w; ++x) {
      const uint32_t baseline_pixel = baseline[baseline_row_offset + x];
      const uint32_t actual_pixel = actual[actual_row_offset + x];
      if (baseline_pixel == actual_pixel) {
        continue;
      }

      if (options.max_pixel_per_channel_delta == 0 ||
          MaxPixelPerChannelDelta(baseline_pixel, actual_pixel) >
              options.max_pixel_per_channel_delta) {
        ++pixels_different;
      }
      if (options.max_mean_squared_error > 0.0) {
        total_squared_error += PixelSquaredError(baseline_pixel, actual_pixel);
      }
    }
  }

  if (options.max_mean_squared_error > 0.0) {
    constexpr double kChannelCount = 3.0;
    const double mse =
        static_cast<double>(total_squared_error) / (kChannelCount * w * h);
    if (mse > options.max_mean_squared_error) {
      ++pixels_different;
    }
  }

  return pixels_different;
}
