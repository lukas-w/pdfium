// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/image_predictors.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <vector>

#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcodec {
namespace {

uint8_t ReferencePaethPredictor(uint8_t a, uint8_t b, uint8_t c) {
  int p = static_cast<int>(a) + b - c;
  int pa = abs(p - a);
  int pb = abs(p - b);
  int pc = abs(p - c);
  if (pa <= pb && pa <= pc) {
    return a;
  }
  return pb <= pc ? b : c;
}

void ReferencePngPredictLine(pdfium::span<uint8_t> dest_span,
                             pdfium::span<const uint8_t> src_span,
                             pdfium::span<const uint8_t> last_span,
                             size_t row_size,
                             uint32_t bytes_per_pixel) {
  const uint8_t tag = src_span.front();
  src_span = src_span.subspan(1u, row_size);
  dest_span = dest_span.first(row_size);
  if (!last_span.empty()) {
    last_span = last_span.first(row_size);
  }
  const size_t bpp = bytes_per_pixel;
  for (size_t i = 0; i < row_size; ++i) {
    const uint8_t left = i >= bpp ? dest_span[i - bpp] : 0;
    const uint8_t up = last_span.empty() ? 0 : last_span[i];
    const uint8_t upper_left =
        (i >= bpp && !last_span.empty()) ? last_span[i - bpp] : 0;
    switch (tag) {
      case 1:
        dest_span[i] = src_span[i] + left;
        break;
      case 2:
        dest_span[i] = src_span[i] + up;
        break;
      case 3:
        dest_span[i] = src_span[i] + (up + left) / 2;
        break;
      case 4:
        dest_span[i] =
            src_span[i] + ReferencePaethPredictor(left, up, upper_left);
        break;
      default:
        dest_span[i] = src_span[i];
        break;
    }
  }
}

}  // namespace

TEST(ImagePredictorsTest, PngPredictLineAllTagsAndWidths) {
  for (uint8_t tag = 0; tag <= 5; ++tag) {
    for (uint32_t bpp : {1u, 2u, 3u, 4u, 6u}) {
      for (size_t row_size :
           {1u, 2u, 3u, 4u, 5u, 7u, 8u, 12u, 16u, 31u, 64u, 129u}) {
        std::vector<uint8_t> src(row_size + 1);
        std::vector<uint8_t> last(row_size);
        src[0] = tag;
        for (size_t i = 0; i < row_size; ++i) {
          src[i + 1] =
              static_cast<uint8_t>((i * 73 + tag * 19 + bpp * 41) & 0xFF);
          last[i] =
              static_cast<uint8_t>((i * 151 + tag * 31 + bpp * 17) & 0xFF);
        }

        // First row (empty last_span).
        std::vector<uint8_t> dst_ref_first(row_size, 0);
        std::vector<uint8_t> dst_opt_first(row_size, 0);
        ReferencePngPredictLine(dst_ref_first, src, {}, row_size, bpp);
        PngPredictLine(dst_opt_first, src, {}, row_size, bpp);
        EXPECT_EQ(dst_ref_first, dst_opt_first)
            << "First row mismatch for tag=" << static_cast<int>(tag)
            << " bpp=" << bpp << " row_size=" << row_size;

        // Subsequent row (non-empty last_span).
        std::vector<uint8_t> dst_ref(row_size, 0);
        std::vector<uint8_t> dst_opt(row_size, 0);
        ReferencePngPredictLine(dst_ref, src, last, row_size, bpp);
        PngPredictLine(dst_opt, src, last, row_size, bpp);
        EXPECT_EQ(dst_ref, dst_opt)
            << "Row mismatch for tag=" << static_cast<int>(tag)
            << " bpp=" << bpp << " row_size=" << row_size;
      }
    }
  }
}

}  // namespace fxcodec
