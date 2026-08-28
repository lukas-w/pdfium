// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/bitmap_saver.h"

#include <limits.h>

#include <fstream>
#include <vector>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "testing/png_codec/png_codec.h"

#ifdef PDF_ENABLE_SKIA
#include "third_party/skia/include/core/SkAlphaType.h"  // nogncheck
#include "third_party/skia/include/core/SkColorType.h"  // nogncheck
#include "third_party/skia/include/core/SkImageInfo.h"  // nogncheck
#include "third_party/skia/include/core/SkPixmap.h"     // nogncheck
#endif

// static
std::vector<uint8_t> BitmapSaver::EncodeBitmapToPng(FPDF_BITMAP bitmap) {
  if (!bitmap) {
    return {};
  }
  const int stride = FPDFBitmap_GetStride(bitmap);
  const int width = FPDFBitmap_GetWidth(bitmap);
  const int height = FPDFBitmap_GetHeight(bitmap);
  if (stride <= 0 || width <= 0 || height <= 0) {
    return {};
  }
  FX_SAFE_SIZE_T size = stride;
  size *= height;
  if (!size.IsValid()) {
    return {};
  }
  auto input = UNSAFE_TODO(
      pdfium::span(static_cast<const uint8_t*>(FPDFBitmap_GetBuffer(bitmap)),
                   size.ValueOrDie()));

  switch (FPDFBitmap_GetFormat(bitmap)) {
    case FPDFBitmap_Gray:
      return png_codec::EncodeGray(input, width, height, stride);
    case FPDFBitmap_BGR:
      return png_codec::EncodeBGR(input, width, height, stride);
    case FPDFBitmap_BGRx:
      return png_codec::EncodeBGRA(input, width, height, stride,
                                   /*discard_transparency=*/true);
    case FPDFBitmap_BGRA:
      return png_codec::EncodeBGRA(input, width, height, stride,
                                   /*discard_transparency=*/false);
#ifdef PDF_ENABLE_SKIA
    case FPDFBitmap_BGRA_Premul: {
      std::vector<uint8_t> straight_alpha_input =
          ConvertToStraightAlpha(input, width, height, stride);
      return png_codec::EncodeBGRA(straight_alpha_input, width, height, stride,
                                   /*discard_transparency=*/false);
    }
#endif
    case FPDFBitmap_Unknown:
      return {};
    default:
      NOTREACHED();
  }
}

// static
void BitmapSaver::WriteBitmapToPng(FPDF_BITMAP bitmap,
                                   const std::string& filename) {
  std::vector<uint8_t> png = EncodeBitmapToPng(bitmap);
  DCHECK(!png.empty());
  DCHECK_LT(filename.size(), 256u);

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

#ifdef PDF_ENABLE_SKIA
// static
std::vector<uint8_t> BitmapSaver::ConvertToStraightAlpha(
    pdfium::span<const uint8_t> input,
    int width,
    int height,
    int row_byte_width) {
  SkImageInfo premul_alpha_image_info = SkImageInfo::Make(
      width, height, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
  SkPixmap src_pixmap(premul_alpha_image_info, input.data(), row_byte_width);

  std::vector<uint8_t> result(input.size());
  SkImageInfo straight_alpha_image_info = SkImageInfo::Make(
      width, height, kBGRA_8888_SkColorType, kUnpremul_SkAlphaType);
  SkPixmap dst_pixmap(straight_alpha_image_info, result.data(), row_byte_width);

  CHECK(src_pixmap.readPixels(dst_pixmap));
  return result;
}
#endif
