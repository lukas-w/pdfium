// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_FX_LIBYUV_WRAPPERS_H_
#define CORE_FXGE_FX_LIBYUV_WRAPPERS_H_

#if !defined(PDF_USE_LIBYUV)
#error "PDF_USE_LIBYUV must be defined."
#endif

#include <stdint.h>

#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

namespace pdfium {

enum RotationMode {
  kRotate90 = 90,
  kRotate270 = 270,
};

// Span-based wrappers around functions in third_party/libyuv/include/libyuv/.
// See the corresponding libyuv headers for detailed documentation.

// 2D plane operations.
void RGB24ToARGB(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height);

void ARGBToRGB24(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height);

void ARGBToRAW(pdfium::span<const uint8_t> src_span,
               int src_stride,
               pdfium::span<uint8_t> dst_span,
               int dst_stride,
               int width,
               int height);

void ARGBExtractAlpha(pdfium::span<const uint8_t> src_span,
                      int src_stride,
                      pdfium::span<uint8_t> dst_span,
                      int dst_stride,
                      int width,
                      int height);

void ARGBCopyYToAlpha(pdfium::span<const uint8_t> src_span,
                      int src_stride,
                      pdfium::span<uint8_t> dst_span,
                      int dst_stride,
                      int width,
                      int height);

// For mirror and rotate functions below, negative `height` inverts the image
// vertically.
void MirrorPlane(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height);

void RGB24Mirror(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height);

void ARGBMirror(pdfium::span<const uint8_t> src_span,
                int src_stride,
                pdfium::span<uint8_t> dst_span,
                int dst_stride,
                int width,
                int height);

void RotatePlane(pdfium::span<const uint8_t> src_span,
                 int src_stride,
                 pdfium::span<uint8_t> dst_span,
                 int dst_stride,
                 int width,
                 int height,
                 RotationMode mode);

void ARGBRotate(pdfium::span<const uint8_t> src_span,
                int src_stride,
                pdfium::span<uint8_t> dst_span,
                int dst_stride,
                int width,
                int height,
                RotationMode mode);

// Single-row operations. Each function processes `src_span.size()` pixels.
// `src_span` must not be empty, and `dst_span` must have at least
// `src_span.size()` elements (any extra elements in `dst_span` are left
// unmodified).
void RGB24ToRAWRow(pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_span,
                   pdfium::span<FX_RGB_STRUCT<uint8_t>> dst_span);

void RGB24ToARGBRow(pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_span,
                    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dst_span);

void RAWToARGBRow(pdfium::span<const FX_RGB_STRUCT<uint8_t>> src_span,
                  pdfium::span<FX_BGRA_STRUCT<uint8_t>> dst_span);

void ARGBToRGB24Row(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
                    pdfium::span<FX_BGR_STRUCT<uint8_t>> dst_span);

void ARGBExtractAlphaRow(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
                         pdfium::span<uint8_t> dst_span);

void MirrorPlaneRow(pdfium::span<const uint8_t> src_span,
                    pdfium::span<uint8_t> dst_span);

void RGB24MirrorRow(pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_span,
                    pdfium::span<FX_BGR_STRUCT<uint8_t>> dst_span);

void ARGBMirrorRow(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
                   pdfium::span<FX_BGRA_STRUCT<uint8_t>> dst_span);

}  // namespace pdfium

#endif  // CORE_FXGE_FX_LIBYUV_WRAPPERS_H_
