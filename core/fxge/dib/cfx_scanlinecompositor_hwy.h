// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_DIB_CFX_SCANLINECOMPOSITOR_HWY_H_
#define CORE_FXGE_DIB_CFX_SCANLINECOMPOSITOR_HWY_H_

#include <stdint.h>

#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

// Highway kernels for the scanline-compositor cases, each bit-exact with
// the scalar reference it replaces. Runtime target selection is internal.

// SIMD replacement for CompositeRowBgra2Bgra(), normal blend, no clip.
void CompositeRowBgra2BgraSimd(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span);

// SIMD replacement for CopyRowToOpaqueBgra(), BGRA source.
void CompositeRowBgra2BgrxForceOpaqueSimd(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span);

#endif  // CORE_FXGE_DIB_CFX_SCANLINECOMPOSITOR_HWY_H_
