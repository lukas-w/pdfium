// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_dibitmap_hwy.h"

#include <stddef.h>
#include <stdint.h>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/zip.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "core/fxge/dib/cfx_dibitmap_hwy.cpp"
#include "third_party/highway/src/hwy/foreach_target.h"  // IWYU pragma: keep

#include "third_party/highway/src/hwy/highway.h"

HWY_BEFORE_NAMESPACE();
namespace fxge_dibitmap_simd {
namespace HWY_NAMESPACE {
namespace {

namespace hn = hwy::HWY_NAMESPACE;

#if HWY_TARGET != HWY_SCALAR
using D32 = hn::ScalableTag<uint32_t>;
using D16 = hn::Repartition<uint16_t, D32>;
using D8 = hn::Repartition<uint8_t, D32>;

// exact x/255 for x <= 65025
HWY_INLINE hn::Vec<D32> Div255(hn::Vec<D32> x) {
  const D32 d;
  return hn::ShiftRight<8>(
      hn::Add(hn::Add(x, hn::Set(d, 1)), hn::ShiftRight<8>(x)));
}

// u16 multiply: alpha in even lanes, odd lanes 0, so odd products are 0.
HWY_INLINE hn::Vec<D32> ScaleAlpha(hn::Vec<D32> v, hn::Vec<D16> f) {
  const D32 d;
  const D16 d16;
  const hn::Vec<D16> a = hn::BitCast(d16, hn::ShiftRight<24>(v));
  const hn::Vec<D32> na = Div255(hn::BitCast(d, hn::Mul(a, f)));
  // Not OrAnd: AVX-512 ternlog blocks the byte-blend fold.
  return hn::Or(hn::And(v, hn::Set(d, 0x00FFFFFFu)), hn::ShiftLeft<24>(na));
}
#endif  // HWY_TARGET != HWY_SCALAR

// alpha = alpha * factor / 255; BGR kept.
void RunMultiplyAlpha(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest,
                      uint8_t factor) {
#if HWY_TARGET != HWY_SCALAR
  const D32 d;
  const D16 d16;
  const D8 d8;
  const size_t n = hn::Lanes(d);  // pixels/vector
  const hn::Vec<D16> f = hn::Set(d16, factor);

  while (dest.size() >= n) {
    const hn::Vec<D32> v =
        hn::BitCast(d, hn::LoadU(d8, pdfium::as_bytes(dest).data()));
    const hn::Vec<D32> out = ScaleAlpha(v, f);
    hn::StoreU(hn::BitCast(d8, out), d8,
               pdfium::as_writable_bytes(dest).data());
    dest = dest.subspan(n);
  }
#endif  // HWY_TARGET != HWY_SCALAR
  for (auto& pixel : dest) {
    pixel.alpha = pixel.alpha * factor / 255;
  }
}

// alpha = alpha * mask / 255, one mask byte per pixel; BGR kept.
void RunMultiplyAlphaMask(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest,
                          pdfium::span<const uint8_t> mask) {
  // Make sure the compiler can see equal span sizes after the dispatch.
  mask = mask.first(dest.size());
#if HWY_TARGET != HWY_SCALAR
  const D32 d;
  const D16 d16;
  const D8 d8;
  const hn::Rebind<uint8_t, D32> d8r;  // promote to u32
  const size_t n = hn::Lanes(d);       // pixels/vector

  while (dest.size() >= n) {
    const hn::Vec<D32> v =
        hn::BitCast(d, hn::LoadU(d8, pdfium::as_bytes(dest).data()));
    const hn::Vec<D16> m =
        hn::BitCast(d16, hn::PromoteTo(d, hn::LoadU(d8r, mask.data())));
    const hn::Vec<D32> out = ScaleAlpha(v, m);
    hn::StoreU(hn::BitCast(d8, out), d8,
               pdfium::as_writable_bytes(dest).data());
    dest = dest.subspan(n);
    mask = mask.subspan(n);
  }
#endif  // HWY_TARGET != HWY_SCALAR
  for (auto [pixel, m] : fxcrt::Zip(dest, mask)) {
    pixel.alpha = pixel.alpha * m / 255;
  }
}

}  // namespace
}  // namespace HWY_NAMESPACE
}  // namespace fxge_dibitmap_simd
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace fxge_dibitmap_simd {

HWY_EXPORT(RunMultiplyAlpha);
HWY_EXPORT(RunMultiplyAlphaMask);

// Unqualified name, in-namespace: a qualified name breaks single-target
// HWY_STATIC_DISPATCH.
void DispatchMultiplyAlpha(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest,
                           uint8_t factor) {
  // SAFETY: dispatch indexes Highway's per-target table; no span macro form.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunMultiplyAlpha)(dest, factor));
}

void DispatchMultiplyAlphaMask(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest,
                               pdfium::span<const uint8_t> mask) {
  // SAFETY: see DispatchMultiplyAlpha().
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunMultiplyAlphaMask)(dest, mask));
}

}  // namespace fxge_dibitmap_simd

void MultiplyAlphaSimd(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span,
                       uint8_t bitmap_alpha) {
  fxge_dibitmap_simd::DispatchMultiplyAlpha(dest_span, bitmap_alpha);
}

void MultiplyAlphaMaskSimd(pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span,
                           pdfium::span<const uint8_t> mask_span) {
  CHECK_LE(dest_span.size(), mask_span.size());
  fxge_dibitmap_simd::DispatchMultiplyAlphaMask(dest_span, mask_span);
}
#endif  // HWY_ONCE
