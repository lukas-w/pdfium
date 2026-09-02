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
};

inline constexpr DiffOptions kExactDiffOptions = {};
inline constexpr DiffOptions kFuzzyDiffOptions = {
    .max_pixel_per_channel_delta = kMaxFuzzyPixelDelta,
    .max_mean_squared_error = kMaxFuzzyMeanSquaredError,
};

// Returns the largest difference in pixel channels between `baseline_pixel` and
// `actual_pixel`. Pixels are expected to be in 32-bit ARGB or BGRA format.
uint8_t MaxPixelPerChannelDelta(uint32_t baseline_pixel, uint32_t actual_pixel);

// Returns the sum of squared differences across color channels (R, G, B)
// between `baseline_pixel` and `actual_pixel`.
uint32_t PixelSquaredError(uint32_t baseline_pixel, uint32_t actual_pixel);

// Computes the maximum local Mean Squared Error across all overlapping
// (`window_size` x `window_size`) windows between two images of dimensions
// (`w`, `h`). Pixels are in 32-bit ARGB or BGRA format.
double CalculateMaxWindowMSE(pdfium::span<const uint32_t> baseline,
                             pdfium::span<const uint32_t> actual,
                             int w,
                             int h,
                             int window_size);

#endif  // TESTING_UTILS_PIXEL_DIFF_UTIL_H_
