// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_scanlinecompositor_hwy.h"

#include <stddef.h>
#include <stdint.h>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"

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

// scalar tail; bit-exact with CompositeRowBgra2Bgra normal/no-clip
inline uint32_t CompositePixelBgra2Bgra(uint32_t src, uint32_t back) {
  const uint32_t back_alpha = FXARGB_A(back);
  if (back_alpha == 0) {
    return src;
  }
  const uint32_t src_alpha = FXARGB_A(src);
  if (src_alpha == 0) {
    return back;
  }
  if (src_alpha == 255) {
    return src;
  }
  const uint32_t dest_alpha =
      back_alpha + src_alpha - (back_alpha * src_alpha / 255);
  const uint32_t ratio = src_alpha * 255 / dest_alpha;
  return ArgbEncode(dest_alpha,
                    AlphaMerge(FXARGB_R(back), FXARGB_R(src), ratio),
                    AlphaMerge(FXARGB_G(back), FXARGB_G(src), ratio),
                    AlphaMerge(FXARGB_B(back), FXARGB_B(src), ratio));
}

// Porter-Duff over, packed BGRA, normal, no clip. Per pixel:
//   dest a == 0 -> copy src     src a == 255 -> copy src
//   src a == 0 -> keep dest     else AlphaMerge/AlphaUnion over
// Byte spans bitcast to u32 lanes -> no 4-byte scanline alignment needed.
void RunBgra2Bgra(pdfium::span<const uint8_t> src, pdfium::span<uint8_t> dest) {
  const hn::ScalableTag<uint32_t> d;
  const hn::ScalableTag<uint8_t> d8;
  const size_t n = hn::Lanes(d);       // pixels/vector
  const auto m255 = hn::Set(d, 0xFF);  // byte mask and the value 255
  const auto one = hn::Set(d, 1);
  const auto zero = hn::Zero(d);

  size_t count = src.size() / 4;
  while (count >= n) {
    const auto s = hn::BitCast(d, hn::LoadU(d8, src.data()));
    const auto back = hn::BitCast(d, hn::LoadU(d8, dest.data()));
    const auto sa = hn::ShiftRight<24>(s);
    const auto da = hn::ShiftRight<24>(back);

    const auto copy_src = hn::Or(hn::Eq(da, zero), hn::Eq(sa, m255));
    const auto keep_dest = hn::Eq(sa, zero);

    // all copy/keep -> skip divide; copy_src ? src : dest
    if (hn::AllTrue(d, hn::Or(copy_src, keep_dest))) {
      const auto result = hn::IfThenElse(copy_src, s, back);
      hn::StoreU(hn::BitCast(d8, result), d8, dest.data());
      src = src.subspan(n * 4);
      dest = dest.subspan(n * 4);
      count -= n;
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
    hn::StoreU(hn::BitCast(d8, result), d8, dest.data());

    src = src.subspan(n * 4);
    dest = dest.subspan(n * 4);
    count -= n;
  }
  for (size_t i = 0; i < count; ++i) {
    const uint32_t s = FXARGB_GetDIB(src.first<4u>());
    const uint32_t back = FXARGB_GetDIB(dest.first<4u>());
    FXARGB_SetDIB(dest.first<4u>(), CompositePixelBgra2Bgra(s, back));
    src = src.subspan(4u);
    dest = dest.subspan(4u);
  }
}

// Opaque BGRA(4) -> BGRA, alpha forced 0xFF (src alpha ignored). Same byte
// order required: src and dest both packed BGRA.
void RunBgra2BgrxForceOpaque(pdfium::span<const uint8_t> src,
                             pdfium::span<uint8_t> dest) {
  const hn::ScalableTag<uint32_t> d;
  const hn::ScalableTag<uint8_t> d8;
  const size_t n = hn::Lanes(d);
  const auto alpha_ff = hn::Set(d, 0xFF000000u);

  size_t count = src.size() / 4;
  while (count >= n) {
    const auto v = hn::BitCast(d, hn::LoadU(d8, src.data()));
    hn::StoreU(hn::BitCast(d8, hn::Or(v, alpha_ff)), d8, dest.data());
    src = src.subspan(n * 4);
    dest = dest.subspan(n * 4);
    count -= n;
  }
  for (size_t i = 0; i < count; ++i) {
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = 0xFF;
    src = src.subspan(4u);
    dest = dest.subspan(4u);
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
void DispatchBgra2Bgra(pdfium::span<const uint8_t> src,
                       pdfium::span<uint8_t> dest) {
  // SAFETY: dispatch indexes Highway's per-target table; no span macro form.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBgra2Bgra)(src, dest));
}

void DispatchBgra2BgrxForceOpaque(pdfium::span<const uint8_t> src,
                                  pdfium::span<uint8_t> dest) {
  // SAFETY: see DispatchBgra2Bgra().
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBgra2BgrxForceOpaque)(src, dest));
}

}  // namespace fxge_composite_simd

void CompositeRowBgra2BgraSimd(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span) {
  // `src_span.size()` pixels, like `fxcrt::Zip(src_span, dest_span)`.
  CHECK_LE(src_span.size(), dest_span.size());
  fxge_composite_simd::DispatchBgra2Bgra(
      fxcrt::reinterpret_span<const uint8_t>(src_span),
      fxcrt::reinterpret_span<uint8_t>(dest_span));
}

void CompositeRowBgra2BgrxForceOpaqueSimd(
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_span) {
  // `src_span.size()` pixels, like `fxcrt::Zip(src_span, dest_span)`.
  CHECK_LE(src_span.size(), dest_span.size());
  fxge_composite_simd::DispatchBgra2BgrxForceOpaque(
      fxcrt::reinterpret_span<const uint8_t>(src_span),
      fxcrt::reinterpret_span<uint8_t>(dest_span));
}
#endif  // HWY_ONCE
