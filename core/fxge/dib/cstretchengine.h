// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_DIB_CSTRETCHENGINE_H_
#define CORE_FXGE_DIB_CSTRETCHENGINE_H_

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <optional>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_memory_wrappers.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/raw_span.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/fx_dib.h"

class CFX_DIBBase;
class PauseIndicatorIface;
class ScanlineComposerIface;

// The horizontal stretch pass's k8BppTo8Bpp and kManyBpptoManyBpp arms (the
// former also serving 1bpp sources, whose rows are expanded to bytes) visit
// every destination column once per source row, and the modal tap counts are
// 1 (no scaling in this axis) and 2 (bilinear interpolation). Four columns
// sharing a tap count, in a long enough run of them, can be resampled in one
// pass with a vectorized gather-and-multiply-accumulate: see
// StretchHorzRunSimd() in cstretchengine_hwy.cpp. PlanHorzGroups() finds those
// runs and, for each whole group of four within one whose combined source
// window fits the gather (at most 32 bytes), precomputes the indices and
// weights the kernel needs. Columns not covered by a group -- an odd tap
// count, a short run, or a group whose window does not fit -- take the
// unchanged per-column path.
struct StretchHorzGroup {
  size_t src_byte_base;  // Byte offset of the source window in the row.
  // Per-tap gather indices into the window; see the kernel.
  std::array<std::array<uint8_t, 16>, 2> idx;
  // Per-tap, per-column weights.
  std::array<std::array<uint32_t, 4>, 2> weights;
  uint32_t taps;  // 1 or 2.
  bool wide;      // Whether the source window needs 32 bytes, not 16.
};
FX_DATA_PARTITION_EXCEPTION(StretchHorzGroup);

// A run of consecutive groups covering 4 * group_count consecutive
// destination columns from dest_col (an absolute column index, the same
// coordinate space ContinueStretchHorz()'s `col` uses): the groups at
// [first_group, first_group + group_count) of horz_groups_. The kernel
// handles a whole run per call.
struct StretchHorzRun {
  int dest_col;
  size_t first_group;
  size_t group_count;
};
FX_DATA_PARTITION_EXCEPTION(StretchHorzRun);

class CStretchEngine {
 public:
  static constexpr uint32_t kFixedPointBits = 16;
  static constexpr uint32_t kFixedPointOne = 1 << kFixedPointBits;

  static inline uint32_t FixedFromDouble(double d) {
    return static_cast<uint32_t>(FXSYS_round(d * kFixedPointOne));
  }

  static inline uint8_t PixelFromFixed(uint32_t fixed) {
    return static_cast<uint8_t>(fixed >> kFixedPointBits);
  }

  // Indicates whether to manually set interpolate bilinear option to true to
  // achieve a smoother rendering results.
  static bool UseInterpolateBilinear(const FXDIB_ResampleOptions& options,
                                     int dest_width,
                                     int dest_height,
                                     int src_width,
                                     int src_height);

  template <typename T>
  static bool IsValidDestinationDimension(T value) {
    // Note: Negate `kMaxImageDimension` in this check, and not the dimension
    // value, since negating the dimension may be undefined behavior.
    static constexpr int kMaxImageDimension = 1024 * 1024;
    return value >= -kMaxImageDimension && value <= kMaxImageDimension;
  }

  struct PixelWeight {
    void SetStartEnd(int src_start, int src_end, size_t weight_count) {
      CHECK_GE(src_start, 0);
      CHECK_LT(src_end - src_start, static_cast<int>(weight_count));
      src_start_ = src_start;
      src_end_ = src_end;
    }

    void SetWeightForPosition(int position, uint32_t weight) {
      CHECK_GE(position, src_start_);
      CHECK_LE(position, src_end_);
      // SAFETY: enforced by checks above.
      UNSAFE_BUFFERS(weights_[position - src_start_] = weight);
    }

    // The first source position this pixel taps. Non-negative, so callers
    // can index a span with it directly.
    size_t GetSrcStart() const { return static_cast<size_t>(src_start_); }

    // One weight per source position in [src_start_, src_end_]; empty for an
    // empty range.
    pdfium::span<const uint32_t> GetWeights() const {
      if (src_end_ < src_start_) {
        return {};
      }
      // SAFETY: CalculateWeights() sizes each PixelWeight for exactly
      // `src_end_ - src_start_ + 1` trailing weights, and SetStartEnd()
      // CHECKs that the range fits the allocation.
      return UNSAFE_BUFFERS(pdfium::span(
          weights_, static_cast<size_t>(src_end_ - src_start_ + 1)));
    }

    // NOTE: relies on defined behaviour for unsigned overflow to
    // decrement the previous position, as needed.
    void RemoveLastWeightAndAdjust(uint32_t weight_change) {
      CHECK_GT(src_end_, src_start_);
      --src_end_;
      // SAFETY: enforced by checks above.
      UNSAFE_BUFFERS(weights_[src_end_ - src_start_] += weight_change);
    }

    int src_start_;
    int src_end_;          // Note: inclusive, [0, -1] for empty range at 0.
    uint32_t weights_[1];  // Not really 1, variable size.
  };

  class WeightTable {
   public:
    WeightTable();
    ~WeightTable();

    // Accepts a negative `dest_len` argument, producing a "mirror
    // image" of the result if `dest_len` is negative.
    bool CalculateWeights(int dest_len,
                          int dest_min,
                          int dest_max,
                          int src_len,
                          int src_min,
                          int src_max,
                          const FXDIB_ResampleOptions& options);

    const PixelWeight* GetPixelWeight(int pixel) const;
    PixelWeight* GetPixelWeight(int pixel);

   private:
    int dest_min_ = 0;
    size_t item_size_bytes_ = 0;
    size_t weight_tables_size_bytes_ = 0;
    DataVector<uint8_t> weight_tables_;
  };

  CStretchEngine(ScanlineComposerIface* pDestBitmap,
                 FXDIB_Format dest_format,
                 int dest_width,
                 int dest_height,
                 const FX_RECT& clip_rect,
                 const RetainPtr<const CFX_DIBBase>& pSrcBitmap,
                 const FXDIB_ResampleOptions& options);
  ~CStretchEngine();

  bool Continue(PauseIndicatorIface* pPause);
  bool StartStretchHorz();
  bool ContinueStretchHorz(PauseIndicatorIface* pPause);
  void StretchVert();

  const FXDIB_ResampleOptions& GetResampleOptionsForTest() const {
    return resample_options_;
  }

 private:
  enum class State : uint8_t { kInitial, kHorizontal, kVertical };

  // ToManyBpp means To24OrTo32Bpp here.
  enum class TransformMethod : uint8_t {
    k1BppTo8Bpp,
    k1BppToManyBpp,
    k8BppTo8Bpp,
    k8BppToManyBpp,
    kManyBpptoManyBpp,
    kManyBpptoManyBppWithAlpha
  };

  // For 1bpp sources: expands `src_row` to one byte per pixel, 0 or 255, in
  // `expanded_row_` and returns that.
  pdfium::span<const uint8_t> Expand1BppRow(
      pdfium::span<const uint8_t> src_row);

  // What PlanHorzGroups() derives once per stretch for its helpers: the
  // source pixel stride the gather reads, how many leading bytes of each
  // pixel it loads, and the byte length of the row the kernel will read.
  struct HorzPlanParams {
    uint32_t bpp;
    uint32_t gathered;
    size_t src_row_bytes;
  };

  // Fills horz_groups_ and horz_runs_ for the current weight table; see
  // StretchHorzGroup.
  void PlanHorzGroups();
  void PlanHorzSpan(int start,
                    int end,
                    uint32_t taps,
                    const HorzPlanParams& params);
  std::optional<StretchHorzGroup>
  BuildHorzGroup(int col, uint32_t taps, const HorzPlanParams& params) const;
  uint32_t HorzTapCountAt(int col) const;
  pdfium::span<const StretchHorzGroup> HorzRunGroups(
      const StretchHorzRun& run) const;

  const FXDIB_Format dest_format_;
  const int dest_bpp_;
  const int src_bpp_;
  const bool has_alpha_;
  RetainPtr<const CFX_DIBBase> const source_;
  pdfium::raw_span<const uint32_t> src_palette_;
  const int src_width_;
  const int src_height_;
  UnownedPtr<ScanlineComposerIface> const dest_bitmap_;
  const int dest_width_;
  const int dest_height_;
  const FX_RECT dest_clip_;
  DataVector<uint8_t> dest_scanline_;
  FixedSizeDataVector<uint8_t> inter_buf_;
  FixedSizeDataVector<uint8_t> expanded_row_;
  FX_RECT src_clip_;
  int inter_pitch_;
  int extra_mask_pitch_;
  FXDIB_ResampleOptions resample_options_;
  TransformMethod trans_method_;
  State state_ = State::kInitial;
  int cur_row_ = 0;
  WeightTable weight_table_;
  DataVector<StretchHorzGroup> horz_groups_;
  DataVector<StretchHorzRun> horz_runs_;
};

#endif  // CORE_FXGE_DIB_CSTRETCHENGINE_H_
