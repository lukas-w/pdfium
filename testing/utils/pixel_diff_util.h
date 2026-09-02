// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTILS_PIXEL_DIFF_UTIL_H_
#define TESTING_UTILS_PIXEL_DIFF_UTIL_H_

#include <stdint.h>

#include "core/fxcrt/span.h"

// Standard fuzzy matching limits.
inline constexpr uint8_t kMaxFuzzyPixelDelta = 3;
inline constexpr double kMaxFuzzyMeanSquaredError = 0.05;
inline constexpr int kMaxFuzzyWindowSize = 8;
inline constexpr double kMaxFuzzyWindowMeanSquaredError = 15.0;

// Options controlling pixel difference comparisons.
struct DiffOptions {
  int max_pixel_per_channel_delta = 0;
  double max_mean_squared_error = 0.0;
  int window_size = 0;
  double max_window_mean_squared_error = 0.0;
};

inline constexpr DiffOptions kExactDiffOptions = {};
inline constexpr DiffOptions kFuzzyDiffOptions = {
    .max_pixel_per_channel_delta = kMaxFuzzyPixelDelta,
    .max_mean_squared_error = kMaxFuzzyMeanSquaredError,
    .window_size = kMaxFuzzyWindowSize,
    .max_window_mean_squared_error = kMaxFuzzyWindowMeanSquaredError,
};

// Returns the largest difference in pixel channels between `baseline_pixel` and
// `actual_pixel`. Pixels are expected to be in 32-bit ARGB or BGRA format.
uint8_t MaxPixelPerChannelDelta(uint32_t baseline_pixel, uint32_t actual_pixel);

// Returns the sum of squared differences across color channels (R, G, B)
// between `baseline_pixel` and `actual_pixel`.
uint32_t PixelSquaredError(uint32_t baseline_pixel, uint32_t actual_pixel);

// Computes the maximum local Mean Squared Error across all overlapping
// (`window_size` x `window_size`) windows between two images of dimensions
// (`w`, `h`) with respective row strides in pixels (`baseline_stride_pixels`,
// `actual_stride_pixels`). Pixels are in 32-bit ARGB or BGRA format.
double CalculateMaxWindowMSE(pdfium::span<const uint32_t> baseline,
                             size_t baseline_stride_pixels,
                             pdfium::span<const uint32_t> actual,
                             size_t actual_stride_pixels,
                             int w,
                             int h,
                             int window_size);

// Returns the number of differing pixels between `baseline` and `actual` within
// the overlap region of dimensions (`w`, `h`), taking into account maximum
// per-channel delta, global mean squared error, and optional windowed mean
// squared error. Pixels are in 32-bit ARGB or BGRA format.
int CalculatePixelsDifferent(pdfium::span<const uint32_t> baseline,
                             size_t baseline_stride_pixels,
                             pdfium::span<const uint32_t> actual,
                             size_t actual_stride_pixels,
                             int w,
                             int h,
                             const DiffOptions& options = kExactDiffOptions);

#endif  // TESTING_UTILS_PIXEL_DIFF_UTIL_H_
