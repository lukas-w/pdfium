// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/fx_libyuv_wrappers.h"

#include <stddef.h>
#include <stdint.h>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_2d_size.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"
#include "third_party/libyuv/include/libyuv/convert_from_argb.h"
#include "third_party/libyuv/include/libyuv/planar_functions.h"
#include "third_party/libyuv/include/libyuv/rotate.h"
#include "third_party/libyuv/include/libyuv/rotate_argb.h"

namespace pdfium {

namespace {

int GetAbsHeight(int height) {
  FX_SAFE_INT32 safe_height = height;
  int abs_height = safe_height.Abs().ValueOrDie();
  CHECK_GT(abs_height, 0);
  return abs_height;
}

template <typename T, typename U>
int ValidateRowWidthsAndGetSrcRowWidth(pdfium::span<const T> src_span,
                                       pdfium::span<U> dst_span) {
  CHECK(!src_span.empty());
  CHECK_LE(src_span.size(), dst_span.size());
  return pdfium::checked_cast<int>(src_span.size());
}

size_t GetPlaneBytes(int stride, int width, int rows, int bytes_per_pixel) {
  CHECK_GT(width, 0);
  CHECK_GT(rows, 0);
  CHECK_GT(bytes_per_pixel, 0);
  const size_t row_bytes = Fx2DSizeOrDie(width, bytes_per_pixel);
  CHECK_GE(stride, 0);
  CHECK_GE(static_cast<size_t>(stride), row_bytes);
  // A subspan starting at a non-zero column offset has `stride` bytes for each
  // of the first `rows - 1` rows, and only `row_bytes` bytes on the final row.
  FX_SAFE_SIZE_T plane_bytes = rows - 1;
  plane_bytes *= stride;
  plane_bytes += row_bytes;
  return plane_bytes.ValueOrDie();
}

libyuv::RotationMode ToLibyuvRotationMode(RotationMode mode) {
  switch (mode) {
    case kRotate90:
      return libyuv::kRotate90;
    case kRotate270:
      return libyuv::kRotate270;
  }
  NOTREACHED();
}

}  // namespace

void RGB24ToARGB(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height) {
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, height, 3));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, height, 4));
  int ret = libyuv::RGB24ToARGB(src_span.data(), src_stride, dst_span.data(),
                                dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void ARGBToRGB24(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height) {
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, height, 4));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, height, 3));
  int ret = libyuv::ARGBToRGB24(src_span.data(), src_stride, dst_span.data(),
                                dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void ARGBToRAW(pdfium::span<const uint8_t> src_span,
               int src_stride,
               pdfium::span<uint8_t> dst_span,
               int dst_stride,
               int width,
               int height) {
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, height, 4));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, height, 3));
  int ret = libyuv::ARGBToRAW(src_span.data(), src_stride, dst_span.data(),
                              dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void ARGBExtractAlpha(pdfium::span<const uint8_t> src_span,
                      int src_stride,
                      pdfium::span<uint8_t> dst_span,
                      int dst_stride,
                      int width,
                      int height) {
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, height, 4));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, height, 1));
  int ret = libyuv::ARGBExtractAlpha(
      src_span.data(), src_stride, dst_span.data(), dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void ARGBCopyYToAlpha(pdfium::span<const uint8_t> src_span,
                      int src_stride,
                      pdfium::span<uint8_t> dst_span,
                      int dst_stride,
                      int width,
                      int height) {
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, height, 1));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, height, 4));
  int ret = libyuv::ARGBCopyYToAlpha(
      src_span.data(), src_stride, dst_span.data(), dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void MirrorPlane(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height) {
  const int abs_height = GetAbsHeight(height);
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, abs_height, 1));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, abs_height, 1));
  libyuv::MirrorPlane(src_span.data(), src_stride, dst_span.data(), dst_stride,
                      width, height);
}

void RGB24Mirror(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height) {
  const int abs_height = GetAbsHeight(height);
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, abs_height, 3));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, abs_height, 3));
  int ret = libyuv::RGB24Mirror(src_span.data(), src_stride, dst_span.data(),
                                dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void ARGBMirror(pdfium::span<const uint8_t> src_span,
                int src_stride,
                pdfium::span<uint8_t> dst_span,
                int dst_stride,
                int width,
                int height) {
  const int abs_height = GetAbsHeight(height);
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, abs_height, 4));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, width, abs_height, 4));
  int ret = libyuv::ARGBMirror(src_span.data(), src_stride, dst_span.data(),
                               dst_stride, width, height);
  CHECK_EQ(ret, 0);
}

void RotatePlane(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height,
                 RotationMode mode) {
  const int abs_height = GetAbsHeight(height);
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, abs_height, 1));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, abs_height, width, 1));
  int ret = libyuv::RotatePlane(src_span.data(), src_stride, dst_span.data(),
                                dst_stride, width, height,
                                ToLibyuvRotationMode(mode));
  CHECK_EQ(ret, 0);
}

void ARGBRotate(pdfium::span<const uint8_t> src_span,
                int src_stride,
                pdfium::span<uint8_t> dst_span,
                int dst_stride,
                int width,
                int height,
                RotationMode mode) {
  const int abs_height = GetAbsHeight(height);
  CHECK_GE(src_span.size(), GetPlaneBytes(src_stride, width, abs_height, 4));
  CHECK_GE(dst_span.size(), GetPlaneBytes(dst_stride, abs_height, width, 4));
  int ret =
      libyuv::ARGBRotate(src_span.data(), src_stride, dst_span.data(),
                         dst_stride, width, height, ToLibyuvRotationMode(mode));
  CHECK_EQ(ret, 0);
}

void RGB24ToRAWRow(pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_span,
                   pdfium::span<FX_RGB_STRUCT<uint8_t>> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::RGB24ToRAW(pdfium::as_bytes(src_span).data(), 0,
                               pdfium::as_writable_bytes(dst_span).data(), 0,
                               width, 1);
  CHECK_EQ(ret, 0);
}

void RGB24ToARGBRow(pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_span,
                    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::RGB24ToARGB(pdfium::as_bytes(src_span).data(), 0,
                                pdfium::as_writable_bytes(dst_span).data(), 0,
                                width, 1);
  CHECK_EQ(ret, 0);
}

void RAWToARGBRow(pdfium::span<const FX_RGB_STRUCT<uint8_t>> src_span,
                  pdfium::span<FX_BGRA_STRUCT<uint8_t>> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::RAWToARGB(pdfium::as_bytes(src_span).data(), 0,
                              pdfium::as_writable_bytes(dst_span).data(), 0,
                              width, 1);
  CHECK_EQ(ret, 0);
}

void ARGBToRGB24Row(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
                    pdfium::span<FX_BGR_STRUCT<uint8_t>> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::ARGBToRGB24(pdfium::as_bytes(src_span).data(), 0,
                                pdfium::as_writable_bytes(dst_span).data(), 0,
                                width, 1);
  CHECK_EQ(ret, 0);
}

void ARGBExtractAlphaRow(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
                         pdfium::span<uint8_t> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::ARGBExtractAlpha(pdfium::as_bytes(src_span).data(), 0,
                                     dst_span.data(), 0, width, 1);
  CHECK_EQ(ret, 0);
}

void MirrorPlaneRow(pdfium::span<const uint8_t> src_span,
                    pdfium::span<uint8_t> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  libyuv::MirrorPlane(src_span.data(), 0, dst_span.data(), 0, width, 1);
}

void RGB24MirrorRow(pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_span,
                    pdfium::span<FX_BGR_STRUCT<uint8_t>> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::RGB24Mirror(pdfium::as_bytes(src_span).data(), 0,
                                pdfium::as_writable_bytes(dst_span).data(), 0,
                                width, 1);
  CHECK_EQ(ret, 0);
}

void ARGBMirrorRow(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
                   pdfium::span<FX_BGRA_STRUCT<uint8_t>> dst_span) {
  const int width = ValidateRowWidthsAndGetSrcRowWidth(src_span, dst_span);
  int ret = libyuv::ARGBMirror(pdfium::as_bytes(src_span).data(), 0,
                               pdfium::as_writable_bytes(dst_span).data(), 0,
                               width, 1);
  CHECK_EQ(ret, 0);
}

}  // namespace pdfium
