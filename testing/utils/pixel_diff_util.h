// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTILS_PIXEL_DIFF_UTIL_H_
#define TESTING_UTILS_PIXEL_DIFF_UTIL_H_

#include <cstdint>

// Standard fuzzy matching limits.
inline constexpr uint8_t kMaxFuzzyPixelDelta = 3;
inline constexpr double kMaxFuzzyMeanSquaredError = 0.05;

// Returns the largest difference in pixel channels between `baseline_pixel` and
// `actual_pixel`. Pixels are expected to be in 32-bit ARGB or BGRA format.
uint8_t MaxPixelPerChannelDelta(uint32_t baseline_pixel, uint32_t actual_pixel);

// Returns the sum of squared differences across color channels (R, G, B)
// between `baseline_pixel` and `actual_pixel`.
uint32_t PixelSquaredError(uint32_t baseline_pixel, uint32_t actual_pixel);

#endif  // TESTING_UTILS_PIXEL_DIFF_UTIL_H_
