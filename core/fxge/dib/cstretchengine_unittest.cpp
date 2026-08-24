// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cstretchengine.h"

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "core/fpdfapi/page/cpdf_dib.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "core/fxge/dib/scanlinecomposer_iface.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Discovered experimentally
constexpr uint32_t kTooBigSrcLen = 20;
constexpr uint32_t kTooBigDestLen = 32 * 1024 * 1024 + 1;

// Captures the scanlines StretchVert() composes, so a test can tell a row that
// was zeroed apart from a copy of the row above it. StretchVert() composes into
// its caller through this interface only, so nothing test-only is needed to
// reach the output.
class ScanlineRecorder final : public ScanlineComposerIface {
 public:
  void ComposeScanline(int line,
                       pdfium::span<const uint8_t> scanline) override {
    rows_[line] = std::vector<uint8_t>(scanline.begin(), scanline.end());
  }

  bool SetInfo(int width,
               int height,
               FXDIB_Format src_format,
               DataVector<uint32_t> src_palette) override {
    return true;
  }

  bool HasRow(int line) const { return rows_.find(line) != rows_.end(); }

  bool RowIsAllZero(int line) const {
    auto it = rows_.find(line);
    CHECK(it != rows_.end());
    return std::ranges::all_of(it->second,
                               [](uint8_t byte) { return byte == 0; });
  }

 private:
  std::map<int, std::vector<uint8_t>> rows_;
};

uint32_t PixelWeightSum(const CStretchEngine::PixelWeight* weights) {
  uint32_t sum = 0;
  for (uint32_t weight : weights->GetWeights()) {
    sum += weight;
  }
  return sum;
}

void ExecuteOneStretchTest(int32_t dest_width,
                           int32_t src_width,
                           const FXDIB_ResampleOptions& options) {
  static constexpr uint32_t kExpectedSum = CStretchEngine::kFixedPointOne;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(dest_width, 0, dest_width, src_width, 0,
                                     src_width, options));
  for (int32_t i = 0; i < dest_width; ++i) {
    EXPECT_EQ(kExpectedSum, PixelWeightSum(table.GetPixelWeight(i)))
        << "for { " << src_width << ", " << dest_width << " } at " << i;
  }
}

void ExecuteOneReversedStretchTest(int32_t dest_width,
                                   int32_t src_width,
                                   const FXDIB_ResampleOptions& options) {
  static constexpr uint32_t kExpectedSum = CStretchEngine::kFixedPointOne;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(-dest_width, 0, dest_width, src_width, 0,
                                     src_width, options));
  for (int32_t i = 0; i < dest_width; ++i) {
    EXPECT_EQ(kExpectedSum, PixelWeightSum(table.GetPixelWeight(i)))
        << "for { " << src_width << ", " << dest_width << " } at " << i
        << " (reversed)";
  }
}

void ExecuteStretchTests(const FXDIB_ResampleOptions& options) {
  // Can't test everything, few random values chosen.
  static constexpr int32_t kDestWidths[] = {1, 2, 337, 512, 808, 2550};
  static constexpr int32_t kSrcWidths[] = {1, 2, 187, 256, 809, 1110};
  for (int32_t src_width : kSrcWidths) {
    for (int32_t dest_width : kDestWidths) {
      ExecuteOneStretchTest(dest_width, src_width, options);
      ExecuteOneReversedStretchTest(dest_width, src_width, options);
    }
  }
}

}  // namespace

TEST(CStretchEngine, OverflowInCtor) {
  FX_RECT clip_rect;
  RetainPtr<CPDF_Dictionary> dict_obj = pdfium::MakeRetain<CPDF_Dictionary>();
  dict_obj->SetNewFor<CPDF_Number>("Width", 71000);
  dict_obj->SetNewFor<CPDF_Number>("Height", 12500);
  RetainPtr<CPDF_Stream> stream =
      pdfium::MakeRetain<CPDF_Stream>(std::move(dict_obj));
  auto dib_source = pdfium::MakeRetain<CPDF_DIB>(nullptr, stream);
  EXPECT_FALSE(dib_source->Load());  // Fail to load due to dimensions.
  CStretchEngine engine(nullptr, FXDIB_Format::k8bppRgb, 500, 500, clip_rect,
                        dib_source, FXDIB_ResampleOptions());
  EXPECT_TRUE(engine.GetResampleOptionsForTest().bInterpolateBilinear);
  EXPECT_FALSE(engine.GetResampleOptionsForTest().bHalftone);
  EXPECT_FALSE(engine.GetResampleOptionsForTest().bNoSmoothing);
  EXPECT_FALSE(engine.GetResampleOptionsForTest().bLossy);
}

TEST(CStretchEngine, WeightRounding) {
  FXDIB_ResampleOptions options;
  ExecuteStretchTests(options);
}

TEST(CStretchEngine, WeightRoundingNoSmoothing) {
  FXDIB_ResampleOptions options;
  options.bNoSmoothing = true;
  ExecuteStretchTests(options);
}

TEST(CStretchEngine, WeightRoundingBilinear) {
  FXDIB_ResampleOptions options;
  options.bInterpolateBilinear = true;
  ExecuteStretchTests(options);
}

TEST(CStretchEngine, WeightRoundingNoSmoothingBilinear) {
  FXDIB_ResampleOptions options;
  options.bNoSmoothing = true;
  options.bInterpolateBilinear = true;
  ExecuteStretchTests(options);
}

TEST(CStretchEngine, ZeroLengthSrc) {
  FXDIB_ResampleOptions options;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(100, 0, 100, 0, 0, 0, options));
}

TEST(CStretchEngine, ZeroLengthSrcNoSmoothing) {
  FXDIB_ResampleOptions options;
  options.bNoSmoothing = true;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(100, 0, 100, 0, 0, 0, options));
}

TEST(CStretchEngine, ZeroLengthSrcBilinear) {
  FXDIB_ResampleOptions options;
  options.bInterpolateBilinear = true;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(100, 0, 100, 0, 0, 0, options));
}

TEST(CStretchEngine, ZeroLengthSrcNoSmoothingBilinear) {
  FXDIB_ResampleOptions options;
  options.bNoSmoothing = true;
  options.bInterpolateBilinear = true;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(100, 0, 100, 0, 0, 0, options));
}

// A source clip box that is empty in x leaves `src_max` at 0, so the
// area-average branch clamps a destination pixel's tap position from above
// against `src_max - 1`, i.e. -1. Regression test for storing that negative
// position; the degenerate entry must contribute no weight.
TEST(CStretchEngine, EmptySourceRangeDownscale) {
  FXDIB_ResampleOptions options;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(1, 0, 1, 100, 0, 0, options));
  EXPECT_EQ(0u, PixelWeightSum(table.GetPixelWeight(0)));
}

// Same degenerate source range, reached through the constructor rather than
// by calling CalculateWeights() directly: a horizontally mirrored
// destination whose clip box maps the source's right edge onto x == 0 leaves
// a source clip of zero width but non-zero height.
TEST(CStretchEngine, MirroredDestinationEmptySourceClip) {
  auto source = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(source->Create(100, 50, FXDIB_Format::kBgra));
  CStretchEngine engine(nullptr, FXDIB_Format::kBgra, /*dest_width=*/-100,
                        /*dest_height=*/50, FX_RECT(100, 0, 200, 50), source,
                        FXDIB_ResampleOptions());
  EXPECT_TRUE(engine.StartStretchHorz());
}

// The mirror image of the case above: here the source clip degenerates at the
// right edge, so `src_min` and `src_max` are both the source width. Clamping
// the tap position up to `src_min` would put it one past the last source
// pixel, which the horizontal pass then reads.
//
// This drives the horizontal pass directly instead of via Continue(), which
// would go on to run StretchVert() and dereference the null
// ScanlineComposerIface*. The tap position is consumed by the horizontal
// pass, so stopping there still covers the case.
TEST(CStretchEngine, EmptySourceClipAtRightEdge) {
  auto source = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(source->Create(100, 50, FXDIB_Format::kBgra));
  CStretchEngine engine(nullptr, FXDIB_Format::kBgra, /*dest_width=*/100,
                        /*dest_height=*/50, FX_RECT(100, 0, 200, 50), source,
                        FXDIB_ResampleOptions());
  ASSERT_TRUE(engine.StartStretchHorz());
  EXPECT_FALSE(engine.ContinueStretchHorz(nullptr));
}

// The vertical pass can be handed an empty tap range even when the source clip
// is not degenerate, which is the case the three tests above cannot reach --
// StartStretchHorz() rejects a zero-height source clip, so `src_min ==
// src_max` never survives to StretchVert().
//
// The geometry that does reach it: a vertically mirrored destination
// (`dest_height < 0`) whose clip box straddles the bottom edge of the
// destination. The constructor maps the clip box back to source space and then
// clamps it with `src_clip_.Intersect()`, raising `src_clip_.top` from -25 to
// 0; the per-row tap position that CalculateWeights() computes afterwards is
// not clamped the same way, so the lower destination rows land above the
// clamped clip and get `src_start = src_min`, `src_end = pixel_pos < src_min`
// -- an empty range.
//
// This matters because StretchVert()'s row-wide accumulator is allocated once
// outside the row loop and reused. A row with no taps writes nothing into it,
// so failing to zero it for that row would repeat the last row that did have
// taps. The source here is opaque white, which makes a repeated row and a
// zeroed row tell apart.
TEST(CStretchEngine, EmptySourceRangeInVerticalPass) {
  auto source = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(source->Create(100, 50, FXDIB_Format::kBgr));
  source->Clear(0xffffffff);

  FXDIB_ResampleOptions options;
  options.bNoSmoothing = true;

  ScanlineRecorder recorder;
  CStretchEngine engine(&recorder, FXDIB_Format::kBgr, /*dest_width=*/100,
                        /*dest_height=*/-50, FX_RECT(0, 25, 100, 75), source,
                        options);
  ASSERT_TRUE(engine.StartStretchHorz());
  ASSERT_FALSE(engine.ContinueStretchHorz(nullptr));
  engine.StretchVert();

  // Destination rows 25..49 tap source rows 24..0 and must be opaque; the
  // clip box runs to 75, and rows 50..74 tap nothing.
  for (int line = 0; line < 25; ++line) {
    ASSERT_TRUE(recorder.HasRow(line)) << "line " << line;
    EXPECT_FALSE(recorder.RowIsAllZero(line)) << "line " << line;
  }
  for (int line = 25; line < 50; ++line) {
    ASSERT_TRUE(recorder.HasRow(line)) << "line " << line;
    EXPECT_TRUE(recorder.RowIsAllZero(line)) << "line " << line;
  }
}

TEST(CStretchEngine, ZeroLengthDest) {
  FXDIB_ResampleOptions options;
  CStretchEngine::WeightTable table;
  ASSERT_TRUE(table.CalculateWeights(0, 0, 0, 100, 0, 100, options));
}

TEST(CStretchEngine, TooManyWeights) {
  FXDIB_ResampleOptions options;
  CStretchEngine::WeightTable table;
  ASSERT_FALSE(table.CalculateWeights(kTooBigDestLen, 0, kTooBigDestLen,
                                      kTooBigSrcLen, 0, kTooBigSrcLen,
                                      options));
}
