// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTILS_BITMAP_SAVER_H_
#define TESTING_UTILS_BITMAP_SAVER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "core/fxcrt/span.h"
#include "public/fpdfview.h"

class CFX_DIBitmap;

class BitmapSaver {
 public:
  static std::vector<uint8_t> EncodeBitmapToPng(FPDF_BITMAP bitmap);
  static void WriteBitmapToPng(FPDF_BITMAP bitmap, const std::string& filename);
  static void WriteBitmapToPng(CFX_DIBitmap* bitmap,
                               const std::string& filename);
#ifdef PDF_ENABLE_SKIA
  static std::vector<uint8_t> ConvertToStraightAlpha(
      pdfium::span<const uint8_t> input,
      int width,
      int height,
      int row_byte_width);
#endif
};

#endif  // TESTING_UTILS_BITMAP_SAVER_H_
