// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cstretchengine_hwy.h"

#include <stddef.h>
#include <stdint.h>

#include <array>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "core/fxge/dib/cstretchengine_hwy.cpp"
#include "third_party/highway/src/hwy/foreach_target.h"  // IWYU pragma: keep
#include "third_party/highway/src/hwy/highway.h"

HWY_BEFORE_NAMESPACE();
namespace fxge_stretch_simd {
namespace HWY_NAMESPACE {
namespace {

namespace hn = hwy::HWY_NAMESPACE;

// HWY_SCALAR has no vector wide enough for Full128<T>: Highway's portable
// fallback for targets without real SIMD is HWY_EMU128 (which does support
// it), and HWY_SCALAR is only selected instead when an embedder defines
// HWY_COMPILE_ONLY_SCALAR, or when HWY_EMU128 itself is broken for their
// compiler. Either way, PlanHorzGroups() (cstretchengine.cpp) checks
// StretchHorzSimdAvailable() and never plans a group in that build, so
// ContinueStretchHorz() never calls these.
#if HWY_TARGET == HWY_SCALAR

void RunBpp1(pdfium::span<const StretchHorzGroup> groups,
             pdfium::span<const uint8_t> src_row,
             pdfium::span<uint8_t> dest) {
  NOTREACHED();
}
void RunBpp3(pdfium::span<const StretchHorzGroup> groups,
             pdfium::span<const uint8_t> src_row,
             pdfium::span<uint8_t> dest) {
  NOTREACHED();
}
void RunBpp4(pdfium::span<const StretchHorzGroup> groups,
             pdfium::span<const uint8_t> src_row,
             pdfium::span<uint8_t> dest) {
  NOTREACHED();
}
void RunAlpha(pdfium::span<const StretchHorzGroup> groups,
              pdfium::span<const uint8_t> src_row,
              pdfium::span<uint8_t> dest) {
  NOTREACHED();
}

#else  // HWY_TARGET == HWY_SCALAR

using D8 = hn::Full128<uint8_t>;
using D32 = hn::Full128<uint32_t>;

// Runs `do_group` over every group in `groups`, each writing the next
// kDestBytes of `dest`. Keeping this loop here, rather than dispatching once
// per group from ContinueStretchHorz(), makes the runtime target selection
// and the call a per-run cost instead of a per-4-columns one: a full-width
// run of an unclipped bilinear row is hundreds of groups.
template <size_t kDestBytes, typename DoGroupFn>
inline void DoRun(pdfium::span<const StretchHorzGroup> groups,
                  pdfium::span<const uint8_t> src_row,
                  pdfium::span<uint8_t> dest,
                  DoGroupFn do_group) {
  for (const StretchHorzGroup& group : groups) {
    do_group(group, src_row, dest.template first<kDestBytes>());
    dest = dest.subspan(kDestBytes);
  }
}

// Gathers one tap's four columns' worth of source bytes into 32-bit lanes,
// zero-extended, per the group's precomputed index plan. `window` is the
// group's 16 or 32 source bytes; `idx` is 16 lane indices into it, where
// unused lanes are the 0xFF sentinel PlanHorzGroups() fills, which must read
// back as zero.
//
// For the narrow (16-byte window) case a single TableLookupBytesOr0() does
// this directly: valid indices are in [0, 16), the sentinel is 0xFF (bit 7
// set), and Or0's contract guarantees zero exactly there.
//
// For the wide (32-byte, two 16-byte table halves) case, Highway's portable
// contract only guarantees a zero when the index's bit 7 is set; indices in
// [16, 128) are implementation-defined. So the low-table lookup needs its
// indices explicitly masked to either the true value (< 16) or 0x80. The
// high-table lookup does not: `idx - 16` wraps every index outside [16, 32)
// -- values below 16, and the 0xFF sentinel -- into [0x80, 0xFF], which is
// already Or0-safe, while valid indices in [16, 32) land correctly in
// [0, 16).
template <bool kWide>
inline hn::Vec<D8> Gather(D8 d8,
                          pdfium::span<const uint8_t, kWide ? 32 : 16> window,
                          const std::array<uint8_t, 16>& idx) {
  const auto raw_idx = hn::LoadU(d8, idx.data());
  if constexpr (kWide) {
    const auto table_lo = hn::LoadU(d8, window.template first<16>().data());
    const auto table_hi =
        hn::LoadU(d8, window.template subspan<16, 16>().data());
    const auto idx_lo = hn::IfThenElse(hn::Lt(raw_idx, hn::Set(d8, 16)),
                                       raw_idx, hn::Set(d8, 0x80));
    const auto idx_hi = hn::Sub(raw_idx, hn::Set(d8, 16));
    return hn::Or(hn::TableLookupBytesOr0(table_lo, idx_lo),
                  hn::TableLookupBytesOr0(table_hi, idx_hi));
  }
  const auto table = hn::LoadU(d8, window.data());
  return hn::TableLookupBytesOr0(table, raw_idx);
}

// The group's source window within the row: 16 or 32 bytes from
// src_byte_base, which PlanHorzGroups() keeps inside the row. A fixed-extent
// span, so the loads within it need no further bounds checks.
template <bool kWide>
inline pdfium::span<const uint8_t, kWide ? 32 : 16> Window(
    const StretchHorzGroup& group,
    pdfium::span<const uint8_t> src_row) {
  return src_row.subspan(group.src_byte_base).template first<kWide ? 32 : 16>();
}

// Exactly the scalar arithmetic the per-column path performs: gather,
// multiply by the tap weight, accumulate across taps in uint32 with
// wraparound, then (acc >> 16) & 0xFF is CStretchEngine::PixelFromFixed().
// `dest` is the group's 4 destination pixels, 4 * kBpp bytes.
template <int kBpp, int kTaps, bool kWide>
inline void DoGroupImpl(const StretchHorzGroup& group,
                        pdfium::span<const uint8_t> src_row,
                        pdfium::span<uint8_t, 4 * kBpp> dest) {
  const D8 d8;
  const D32 d32;
  const auto window = Window<kWide>(group, src_row);

  const auto p0 = hn::BitCast(d32, Gather<kWide>(d8, window, group.idx[0]));
  const auto w0 = hn::LoadU(d32, group.weights[0].data());
  const auto m255 = hn::Set(d32, 0xFF);

  hn::Vec<D32> b{};
  hn::Vec<D32> g{};
  hn::Vec<D32> r{};
  if constexpr (kBpp == 1) {
    b = hn::Mul(p0, w0);
  } else {
    b = hn::Mul(hn::And(p0, m255), w0);
    g = hn::Mul(hn::And(hn::ShiftRight<8>(p0), m255), w0);
    r = hn::Mul(hn::ShiftRight<16>(p0), w0);
  }

  if constexpr (kTaps == 2) {
    const auto p1 = hn::BitCast(d32, Gather<kWide>(d8, window, group.idx[1]));
    const auto w1 = hn::LoadU(d32, group.weights[1].data());
    if constexpr (kBpp == 1) {
      b = hn::Add(b, hn::Mul(p1, w1));
    } else {
      b = hn::Add(b, hn::Mul(hn::And(p1, m255), w1));
      g = hn::Add(g, hn::Mul(hn::And(hn::ShiftRight<8>(p1), m255), w1));
      r = hn::Add(r, hn::Mul(hn::ShiftRight<16>(p1), w1));
    }
  }

  // (acc >> 16) & 0xFF is PixelFromFixed(); the mask also satisfies
  // U8FromU32()'s precondition that lanes are already in [0, 256).
  const auto b16 = hn::And(hn::ShiftRight<16>(b), m255);

  // A single if-constexpr/else chain: code after an early return inside one
  // arm of `if constexpr` is still instantiated for every kBpp, so the
  // kBpp == 4 arm's static_assert would otherwise fire for kBpp == 1 too.
  if constexpr (kBpp == 1) {
    const hn::Full32<uint8_t> d8x4;
    hn::StoreU(hn::U8FromU32(b16), d8x4, dest.data());
  } else {
    const auto g16 = hn::And(hn::ShiftRight<16>(g), m255);
    const auto r16 = hn::And(hn::ShiftRight<16>(r), m255);
    // Each 32-bit lane becomes one column's B | G << 8 | R << 16, which in
    // memory is the destination's B, G, R byte order -- the same
    // little-endian lane layout the gather's BitCast to 32-bit lanes above
    // already relies on for splitting the channels out.
    const auto bgr =
        hn::Or(hn::Or(b16, hn::ShiftLeft<8>(g16)), hn::ShiftLeft<16>(r16));
    if constexpr (kBpp == 3) {
      // Drop each lane's zero 4th byte: the 4 columns' 12 payload bytes end
      // up in the vector's first 12 bytes, stored as 8 + 4 so that nothing is
      // written past the 12 bytes the group owns.
      static constexpr std::array<uint8_t, 16> kPack = {
          0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, 0x80, 0x80, 0x80, 0x80};
      const auto packed = hn::TableLookupBytesOr0(hn::BitCast(d8, bgr),
                                                  hn::LoadU(d8, kPack.data()));
      const hn::Half<D8> d8x8;
      hn::StoreU(hn::LowerHalf(d8x8, packed), d8x8, dest.data());
      const uint32_t tail = hn::ExtractLane(hn::BitCast(d32, packed), 2);
      fxcrt::spancpy(dest.template subspan<8, 4>(),
                     pdfium::byte_span_from_ref(tail));
    } else {
      static_assert(kBpp == 4);
      // Byte 3 (alpha/padding) is left untouched, matching the per-column
      // path: the 16 bytes are written back with their current 4th bytes.
      const auto old = hn::BitCast(d32, hn::LoadU(d8, dest.data()));
      const auto out = hn::Or(hn::And(old, hn::Set(d32, 0xFF000000u)), bgr);
      hn::StoreU(hn::BitCast(d8, out), d8, dest.data());
    }
  }
}

// Chooses among the 4 (taps, wide) instantiations of DoGroupImpl<kBpp, ...>
// once per group of 4 columns -- not once per column, so this one small
// branch is negligible next to the gather and multiply-accumulate it
// selects.
template <int kBpp>
inline void DoGroup(const StretchHorzGroup& group,
                    pdfium::span<const uint8_t> src_row,
                    pdfium::span<uint8_t, 4 * kBpp> dest) {
  if (group.taps == 1) {
    if (group.wide) {
      DoGroupImpl<kBpp, 1, true>(group, src_row, dest);
    } else {
      DoGroupImpl<kBpp, 1, false>(group, src_row, dest);
    }
  } else {
    if (group.wide) {
      DoGroupImpl<kBpp, 2, true>(group, src_row, dest);
    } else {
      DoGroupImpl<kBpp, 2, false>(group, src_row, dest);
    }
  }
}

// Same shape as DoGroupImpl(), for the alpha arm: source and destination are
// always 4-byte BGRA, and each tap's contribution is weighted by
// `weight * src.alpha / 255`, not just `weight` -- a per-lane divide that
// depends on the gathered source alpha, so it can't be folded into the
// precomputed per-tap weight the non-alpha kernel uses. `weight * alpha`
// reaches 65536 * 255, so the usual `(x + 1 + (x >> 8)) >> 8` approximation
// (exact only up to 65535) is wrong here; instead this is the exact
// reciprocal-multiply trick for unsigned division by 255, matching the
// scalar path's `weight * src.alpha / 255` bit for bit: `(x * 0x80808081) >>
// 39` equals `x / 255` for every uint32_t `x`, including this kernel's full
// range (verified exhaustively over [0, 65536 * 255]), and `MulHigh` +
// `ShiftRight<7>` compute exactly that -- the top 32 bits of the 64-bit
// product, then the remaining 7 of the 39 shift -- without `Div`'s
// float-conversion round trip.
template <int kTaps, bool kWide>
inline void DoGroupAlphaImpl(const StretchHorzGroup& group,
                             pdfium::span<const uint8_t> src_row,
                             pdfium::span<uint8_t, 16> dest) {
  const D8 d8;
  const D32 d32;
  const auto window = Window<kWide>(group, src_row);
  const auto m255 = hn::Set(d32, 0xFF);
  const auto kMul255Recip = hn::Set(d32, 0x80808081u);

  const auto p0 = hn::BitCast(d32, Gather<kWide>(d8, window, group.idx[0]));
  const auto w0 = hn::LoadU(d32, group.weights[0].data());
  const auto alpha0 = hn::ShiftRight<24>(p0);
  const auto pw0 =
      hn::ShiftRight<7>(hn::MulHigh(hn::Mul(w0, alpha0), kMul255Recip));
  auto b = hn::Mul(pw0, hn::And(p0, m255));
  auto g = hn::Mul(pw0, hn::And(hn::ShiftRight<8>(p0), m255));
  auto r = hn::Mul(pw0, hn::And(hn::ShiftRight<16>(p0), m255));
  auto a = pw0;

  if constexpr (kTaps == 2) {
    const auto p1 = hn::BitCast(d32, Gather<kWide>(d8, window, group.idx[1]));
    const auto w1 = hn::LoadU(d32, group.weights[1].data());
    const auto alpha1 = hn::ShiftRight<24>(p1);
    const auto pw1 =
        hn::ShiftRight<7>(hn::MulHigh(hn::Mul(w1, alpha1), kMul255Recip));
    b = hn::Add(b, hn::Mul(pw1, hn::And(p1, m255)));
    g = hn::Add(g, hn::Mul(pw1, hn::And(hn::ShiftRight<8>(p1), m255)));
    r = hn::Add(r, hn::Mul(pw1, hn::And(hn::ShiftRight<16>(p1), m255)));
    a = hn::Add(a, pw1);
  }

  // Matches the scalar path: blue/green/red are PixelFromFixed() of the
  // accumulated `pixel_weight * channel`; alpha is PixelFromFixed() of
  // `255 * accumulated pixel_weight` (the earlier /255 divides are folded
  // back out here, the same as the per-column loop does).
  // Each 32-bit lane becomes one column's B | G << 8 | R << 16 | A << 24,
  // the destination's BGRA byte order in memory (little-endian lanes, as the
  // gather's BitCast above already assumes), so the 4 columns are one
  // 16-byte store.
  const auto b16 = hn::And(hn::ShiftRight<16>(b), m255);
  const auto g16 = hn::And(hn::ShiftRight<16>(g), m255);
  const auto r16 = hn::And(hn::ShiftRight<16>(r), m255);
  const auto a16 = hn::And(hn::ShiftRight<16>(hn::Mul(a, m255)), m255);
  const auto bgra =
      hn::Or(hn::Or(b16, hn::ShiftLeft<8>(g16)),
             hn::Or(hn::ShiftLeft<16>(r16), hn::ShiftLeft<24>(a16)));
  hn::StoreU(hn::BitCast(d8, bgra), d8, dest.data());
}

inline void DoGroupAlpha(const StretchHorzGroup& group,
                         pdfium::span<const uint8_t> src_row,
                         pdfium::span<uint8_t, 16> dest) {
  if (group.taps == 1) {
    if (group.wide) {
      DoGroupAlphaImpl<1, true>(group, src_row, dest);
    } else {
      DoGroupAlphaImpl<1, false>(group, src_row, dest);
    }
  } else {
    if (group.wide) {
      DoGroupAlphaImpl<2, true>(group, src_row, dest);
    } else {
      DoGroupAlphaImpl<2, false>(group, src_row, dest);
    }
  }
}

void RunBpp1(pdfium::span<const StretchHorzGroup> groups,
             pdfium::span<const uint8_t> src_row,
             pdfium::span<uint8_t> dest) {
  DoRun<4>(groups, src_row, dest, DoGroup<1>);
}
void RunBpp3(pdfium::span<const StretchHorzGroup> groups,
             pdfium::span<const uint8_t> src_row,
             pdfium::span<uint8_t> dest) {
  DoRun<12>(groups, src_row, dest, DoGroup<3>);
}
void RunBpp4(pdfium::span<const StretchHorzGroup> groups,
             pdfium::span<const uint8_t> src_row,
             pdfium::span<uint8_t> dest) {
  DoRun<16>(groups, src_row, dest, DoGroup<4>);
}
void RunAlpha(pdfium::span<const StretchHorzGroup> groups,
              pdfium::span<const uint8_t> src_row,
              pdfium::span<uint8_t> dest) {
  DoRun<16>(groups, src_row, dest, DoGroupAlpha);
}

#endif  // HWY_TARGET == HWY_SCALAR

}  // namespace
}  // namespace HWY_NAMESPACE
}  // namespace fxge_stretch_simd
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace fxge_stretch_simd {

HWY_EXPORT(RunBpp1);
HWY_EXPORT(RunBpp3);
HWY_EXPORT(RunBpp4);
HWY_EXPORT(RunAlpha);

// Called unqualified, from inside this namespace: when only one target is
// compiled (e.g. HWY_SCALAR), HWY_DYNAMIC_DISPATCH()/HWY_EXPORT() collapse to
// a direct call to HWY_STATIC_NAMESPACE::FUNC_NAME, which only resolves if
// FUNC_NAME is looked up from inside the enclosing project namespace, as
// Highway's own usage example does -- not from the StretchHorz*Simd() free
// functions below, which callers reach without naming this namespace.
void CallRunBpp1(pdfium::span<const StretchHorzGroup> groups,
                 pdfium::span<const uint8_t> src_row,
                 pdfium::span<uint8_t> dest_row) {
  // SAFETY: indexes Highway's per-target function table with the index of
  // the target Highway itself chose for this CPU, which it keeps within
  // that table; there is no span form of the macro.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBpp1)(groups, src_row, dest_row));
}
void CallRunBpp3(pdfium::span<const StretchHorzGroup> groups,
                 pdfium::span<const uint8_t> src_row,
                 pdfium::span<uint8_t> dest_row) {
  // SAFETY: indexes Highway's per-target function table with the index of
  // the target Highway itself chose for this CPU, which it keeps within
  // that table; there is no span form of the macro.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBpp3)(groups, src_row, dest_row));
}
void CallRunBpp4(pdfium::span<const StretchHorzGroup> groups,
                 pdfium::span<const uint8_t> src_row,
                 pdfium::span<uint8_t> dest_row) {
  // SAFETY: indexes Highway's per-target function table with the index of
  // the target Highway itself chose for this CPU, which it keeps within
  // that table; there is no span form of the macro.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunBpp4)(groups, src_row, dest_row));
}
void CallRunAlpha(pdfium::span<const StretchHorzGroup> groups,
                  pdfium::span<const uint8_t> src_row,
                  pdfium::span<uint8_t> dest_row) {
  // SAFETY: indexes Highway's per-target function table with the index of
  // the target Highway itself chose for this CPU, which it keeps within
  // that table; there is no span form of the macro.
  UNSAFE_BUFFERS(HWY_DYNAMIC_DISPATCH(RunAlpha)(groups, src_row, dest_row));
}

}  // namespace fxge_stretch_simd

void StretchHorzRunSimd(pdfium::span<const StretchHorzGroup> groups,
                        pdfium::span<const uint8_t> src_row,
                        pdfium::span<uint8_t> dest_row) {
  fxge_stretch_simd::CallRunBpp1(groups, src_row, dest_row);
}

void StretchHorzRunSimd(pdfium::span<const StretchHorzGroup> groups,
                        pdfium::span<const FX_BGR_STRUCT<uint8_t>> src_row,
                        pdfium::span<FX_BGR_STRUCT<uint8_t>> dest_row) {
  fxge_stretch_simd::CallRunBpp3(
      groups, fxcrt::reinterpret_span<const uint8_t>(src_row),
      fxcrt::reinterpret_span<uint8_t>(dest_row));
}

void StretchHorzRunSimd(pdfium::span<const StretchHorzGroup> groups,
                        pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_row,
                        pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_row) {
  fxge_stretch_simd::CallRunBpp4(
      groups, fxcrt::reinterpret_span<const uint8_t>(src_row),
      fxcrt::reinterpret_span<uint8_t>(dest_row));
}

void StretchHorzRunAlphaSimd(
    pdfium::span<const StretchHorzGroup> groups,
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_row,
    pdfium::span<FX_BGRA_STRUCT<uint8_t>> dest_row) {
  fxge_stretch_simd::CallRunAlpha(
      groups, fxcrt::reinterpret_span<const uint8_t>(src_row),
      fxcrt::reinterpret_span<uint8_t>(dest_row));
}

bool StretchHorzSimdAvailable() {
  return (HWY_TARGETS & HWY_SCALAR) == 0;
}
#endif  // HWY_ONCE
