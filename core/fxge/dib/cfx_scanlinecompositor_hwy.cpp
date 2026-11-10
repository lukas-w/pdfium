// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_scanlinecompositor_hwy.h"

#include <stddef.h>
#include <stdint.h>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/zip.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "core/fxge/dib/cfx_scanlinecompositor_hwy.cpp"
#include "third_party/highway/src/hwy/foreach_target.h"  // IWYU pragma: keep

#include "third_party/highway/src/hwy/highway.h"

HWY_BEFORE_NAMESPACE();
namespace fxge_composite_simd {
namespace HWY_NAMESPACE {
namespace {

namespace hn = hwy::HWY_NAMESPACE;

// exact floor(x/255), x in [0, 65025]
template <class V>
HWY_INLINE V Div255(V x) {
  const hn::DFromV<V> d;
  return hn::ShiftRight<8>(
      hn::Add(hn::Add(x, hn::Set(d, 1)), hn::ShiftRight<8>(x)));
}

// (back*(255-a) + src*a)/255; operands <= 255 -> sum <= 65025, Div255 exact
template <class V>
HWY_INLINE V AlphaMergeLanes(V back, V src, V alpha, V v255) {
  return Div255(
      hn::Add(hn::Mul(back, hn::Sub(v255, alpha)), hn::Mul(src, alpha)));
}

// scalar tail; bit-exact with CompositeRowBgra2Bgra() normal/no-clip
inline void CompositePixelBgra2Bgra(const FX_BGRA_STRUCT<uint8_t>& src,
                                    FX_BGRA_STRUCT<uint8_t>& dest) {
  if (dest.alpha == 0) {
    dest = src;
    return;
  }
  if (src.alpha == 0) {
    return;
  }
  if (src.alpha == 255) {
    dest = src;
    return;
  }
  const uint32_t dest_alpha =
      dest.alpha + src.alpha - (dest.alpha * src.alpha / 255);
  const uint32_t ratio = src.alpha * 255 / dest_alpha;
  dest.blue = AlphaMerge(dest.blue, src.blue, ratio);
  dest.green = AlphaMerge(dest.green, src.green, ratio);
  dest.red = AlphaMerge(dest.red, src.red, ratio);
  dest.alpha = dest_alpha;
}

// Porter-Duff over, packed BGRA, normal, no clip. Per pixel:
//   dest a == 0 -> copy src     src a == 255 -> copy src
//   src a == 0 -> keep dest     else AlphaMerge/AlphaUnion over
// Byte loads bitcast to u32 lanes -> no 4-byte scanline alignment needed.
void RunBgra2Bgra(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src,
                  pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest) {
#if HWY_TARGET != HWY_SCALAR
  const hn::ScalableTag<uint32_t> d;
  const hn::ScalableTag<uint8_t> d8;
  const size_t n = hn::Lanes(d);       // pixels/vector
  const auto m255 = hn::Set(d, 0xFF);  // byte mask and the value 255
  const auto one = hn::Set(d, 1);
  const auto zero = hn::Zero(d);

  while (src.size() >= n) {
    const auto s = hn::BitCast(
        d, hn::LoadU(d8, reinterpret_cast<const uint8_t*>(src.data())));
    const auto back = hn::BitCast(
        d, hn::LoadU(d8, reinterpret_cast<const uint8_t*>(dest.data())));
    const auto sa = hn::ShiftRight<24>(s);
    const auto da = hn::ShiftRight<24>(back);

    const auto copy_src = hn::Or(hn::Eq(da, zero), hn::Eq(sa, m255));
    const auto keep_dest = hn::Eq(sa, zero);

    // all copy/keep -> skip divide; copy_src ? src : dest
    if (hn::AllTrue(d, hn::Or(copy_src, keep_dest))) {
      const auto result = hn::IfThenElse(copy_src, s, back);
      hn::StoreU(hn::BitCast(d8, result), d8,
                 reinterpret_cast<uint8_t*>(dest.data()));
      src = src.subspan(n);
      dest = dest.subspan(n);
      continue;
    }

    // divisor forced >= 1: copy/keep lanes may yield da2 == 0 (unused)
    const auto da2 = hn::Sub(hn::Add(da, sa), Div255(hn::Mul(da, sa)));
    const auto ratio = hn::Div(hn::Mul(sa, m255), hn::Max(da2, one));

    const auto db = hn::And(back, m255);
    const auto dg = hn::And(hn::ShiftRight<8>(back), m255);
    const auto dr = hn::And(hn::ShiftRight<16>(back), m255);
    const auto sb = hn::And(s, m255);
    const auto sg = hn::And(hn::ShiftRight<8>(s), m255);
    const auto sr = hn::And(hn::ShiftRight<16>(s), m255);
    const auto nb = AlphaMergeLanes(db, sb, ratio, m255);
    const auto ng = AlphaMergeLanes(dg, sg, ratio, m255);
    const auto nr = AlphaMergeLanes(dr, sr, ratio, m255);
    const auto gen =
        hn::Or(hn::Or(nb, hn::ShiftLeft<8>(ng)),
               hn::Or(hn::ShiftLeft<16>(nr), hn::ShiftLeft<24>(da2)));

    auto result = hn::IfThenElse(keep_dest, back, gen);
    result = hn::IfThenElse(copy_src, s, result);
    hn::StoreU(hn::BitCast(d8, result), d8,
               reinterpret_cast<uint8_t*>(dest.data()));

    src = src.subspan(n);
    dest = dest.subspan(n);
  }
#endif  // HWY_TARGET != HWY_SCALAR
  for (auto [src_elem, dest_elem] : fxcrt::Zip(src, dest)) {
    CompositePixelBgra2Bgra(src_elem, dest_elem);
  }
}

// Opaque BGRA(4) -> BGRA, alpha forced 0xFF (src alpha ignored). Same byte
// order required: src and dest both packed BGRA.
void RunBgra2BgrxForceOpaque(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src,
                             pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest) {
#if HWY_TARGET != HWY_SCALAR
  const hn::ScalableTag<uint32_t> d;
  const hn::ScalableTag<uint8_t> d8;
  const size_t n = hn::Lanes(d);
  const auto alpha_ff = hn::Set(d, 0xFF000000u);

  while (src.size() >= n) {
    const auto v = hn::BitCast(
        d, hn::LoadU(d8, reinterpret_cast<const uint8_t*>(src.data())));
    hn::StoreU(hn::BitCast(d8, hn::Or(v, alpha_ff)), d8,
               reinterpret_cast<uint8_t*>(dest.data()));
    src = src.subspan(n);
    dest = dest.subspan(n);
  }
#endif  // HWY_TARGET != HWY_SCALAR
  for (auto [src_elem, dest_elem] : fxcrt::Zip(src, dest)) {
    dest_elem.blue = src_elem.blue;
    dest_elem.green = src_elem.green;
    dest_elem.red = src_elem.red;
    dest_elem.alpha = 0xFF;
  }
}

}  // namespace
}  // namespace HWY_NAMESPACE
}  // namespace fxge_composite_simd
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace fxge_composite_simd {

HWY_EXPORT(RunBgra2Bgra);
HWY_EXPORT(RunBgra2BgrxForceOpaque);

// Unqualified name, in-namespace: a qualified name breaks single-target
// HWY_STATIC_DISPATCH.
void DispatchBgra2Bgra(pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src,
                       pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest) {
  // SAFETY: dispatch indexes Highway's per-target table; no span macro form.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBgra2Bgra)(src, dest));
}

void DispatchBgra2BgrxForceOpaque(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest) {
  // SAFETY: see DispatchBgra2Bgra().
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBgra2BgrxForceOpaque)(src, dest));
}

}  // namespace fxge_composite_simd

void CompositeRowBgra2BgraSimd(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span) {
  // `src_span.size()` pixels, like `fxcrt::Zip(src_span, dest_span)`.
  CHECK_LE(src_span.size(), dest_span.size());
  fxge_composite_simd::DispatchBgra2Bgra(src_span, dest_span);
}

void CompositeRowBgra2BgrxForceOpaqueSimd(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span) {
  // `src_span.size()` pixels, like `fxcrt::Zip(src_span, dest_span)`.
  CHECK_LE(src_span.size(), dest_span.size());
  fxge_composite_simd::DispatchBgra2BgrxForceOpaque(src_span, dest_span);
}
#endif  // HWY_ONCE
