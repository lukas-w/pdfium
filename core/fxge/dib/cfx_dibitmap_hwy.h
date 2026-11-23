// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_DIB_CFX_DIBITMAP_HWY_H_
#define CORE_FXGE_DIB_CFX_DIBITMAP_HWY_H_

#include <stdint.h>

#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

// Highway kernels for the kBgra alpha-scaling cases, each bit-exact with the
// scalar reference it replaces. Only the alpha byte changes; BGR bytes are
// preserved. Runtime target selection is internal.

// SIMD replacement for the kBgra CFX_DIBitmap::MultiplyAlpha() inner loop:
// alpha <- alpha * bitmap_alpha / 255.
void MultiplyAlphaSimd(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span,
                       uint8_t bitmap_alpha);

// SIMD replacement for the kBgra CFX_DIBitmap::MultiplyAlphaMask() inner loop:
// alpha <- alpha * mask / 255, one mask byte per pixel.
void MultiplyAlphaMaskSimd(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span,
                           pdfium::span<const uint8_t> mask_span);

#endif  // CORE_FXGE_DIB_CFX_DIBITMAP_HWY_H_
