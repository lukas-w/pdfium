// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/bitmap_saver.h"

#include <fstream>
#include <vector>

#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_safe_types.h"
#include "testing/utils/png_encode.h"

// static
void BitmapSaver::WriteBitmapToPng(FPDF_BITMAP bitmap,
                                   const std::string& filename) {
  const int stride = FPDFBitmap_GetStride(bitmap);
  const int width = FPDFBitmap_GetWidth(bitmap);
  const int height = FPDFBitmap_GetHeight(bitmap);
  CHECK(stride >= 0);
  CHECK(width >= 0);
  CHECK(height >= 0);
  FX_SAFE_FILESIZE size = stride;
  size *= height;
  auto input =
      pdfium::span(static_cast<const uint8_t*>(FPDFBitmap_GetBuffer(bitmap)),
                   pdfium::ValueOrDieForType<size_t>(size));

  std::vector<uint8_t> png =
      EncodePng(input, width, height, stride, FPDFBitmap_GetFormat(bitmap));
  DCHECK(!png.empty());
  DCHECK(filename.size() < 256u);

  std::ofstream png_file;
  png_file.open(filename, std::ios_base::out | std::ios_base::binary);
  png_file.write(reinterpret_cast<char*>(&png.front()), png.size());
  DCHECK(png_file.good());
  png_file.close();
}

// static
void BitmapSaver::WriteBitmapToPng(CFX_DIBitmap* bitmap,
                                   const std::string& filename) {
  WriteBitmapToPng(reinterpret_cast<FPDF_BITMAP>(bitmap), filename);
}
