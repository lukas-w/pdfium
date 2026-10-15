// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_DIB_CSTRETCHENGINE_HWY_H_
#define CORE_FXGE_DIB_CSTRETCHENGINE_HWY_H_

#include <stdint.h>

#include "core/fxcrt/span.h"
#include "core/fxge/dib/cstretchengine.h"
#include "core/fxge/dib/fx_dib.h"

// Resamples a run of groups' destination columns, 4 per group, in one call:
// for each group, exactly the scalar arithmetic the per-column path performs
// for the same columns (see PlanHorzGroups() and the callers in
// ContinueStretchHorz()): gather each tap's source bytes from `src_row` via
// the group's precomputed index plan, multiply by the tap weight, accumulate
// across taps in the same order, and write one destination pixel per column.
//
// Every group in `groups` is processed, in order, each writing the next 4
// pixels of `dest_row`, so `dest_row` must hold at least 4 pixels per group.
// PlanHorzGroups() built `groups` for `src_row`, and guarantees each group's
// source window lies within it.
//
// Three destination pixel types, matching the arms of ContinueStretchHorz():
// a byte (k8BppTo8Bpp, including expanded 1bpp rows), FX_BGR_STRUCT
// (kManyBpptoManyBpp to kBgr) and FX_BGRA_STRUCT (kManyBpptoManyBpp to
// kBgrx, whose 4th byte is left untouched, as the per-column path does).
//
// Implemented with Highway (see cstretchengine_hwy.cpp). `group.taps` and
// `group.wide` select among 4 internal code paths once per group of 4
// columns, not once per column.
void StretchHorzRunSimd(pdfium::span<const StretchHorzGroup> groups,
                        pdfium::span<const uint8_t> src_row,
                        pdfium::span<uint8_t> dest_row);
void StretchHorzRunSimd(pdfium::span<const StretchHorzGroup> groups,
                        pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_row,
                        pdfium::span<FX_BGR_STRUCT<uint8_t>> dest_row);
void StretchHorzRunSimd(pdfium::span<const StretchHorzGroup> groups,
                        pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_row,
                        pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_row);

// Same contract, for kManyBpptoManyBppWithAlpha: source and destination are
// BGRA, and each tap's contribution is `weight * src.alpha / 255`, not just
// `weight`, so this isn't one of the overloads above -- it has its own kernel
// (cstretchengine_hwy.cpp) that computes that per-lane divide, and it writes
// all 4 bytes of each destination pixel.
void StretchHorzRunAlphaSimd(
    pdfium::span<const StretchHorzGroup> groups,
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_row,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_row);

// False if this build has no vectorized kernel to dispatch to (HWY_SCALAR,
// see cstretchengine_hwy.cpp), in which case PlanHorzGroups() should not
// plan any groups: there would be nothing able to process them.
bool StretchHorzSimdAvailable();

#endif  // CORE_FXGE_DIB_CSTRETCHENGINE_HWY_H_
