// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cstretchengine.h"

#include <algorithm>
#include <array>
#include <map>
#include <utility>
#include <vector>

#include "core/fpdfapi/page/cpdf_dib.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/containers/to_vector.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/cstretchengine_hwy.h"
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
    rows_[line] = pdfium::ToVector(scanline);
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

namespace {

// What one group of StretchHorzRunSimd() is documented to compute (see the
// comment above StretchHorzGroup), re-derived in plain scalar code rather
// than copied from the kernel: per column and channel, gather each tap's byte
// through the group's index plan (a 0xFF index reads as zero), multiply by
// the tap's weight, accumulate in uint32 with wraparound, take bits 16..23.
void ScalarGroupReference(int bpp,
                          const StretchHorzGroup& group,
                          pdfium::span<const uint8_t> src_row,
                          std::array<std::array<uint8_t, 3>, 4>& expected) {
  const int gathered = bpp == 1 ? 1 : 3;
  for (int k = 0; k < 4; ++k) {
    for (int c = 0; c < gathered; ++c) {
      uint32_t acc = 0;
      for (uint32_t t = 0; t < group.taps; ++t) {
        const uint8_t idx = group.idx[t][k * 4 + c];
        const uint8_t byte_val =
            idx == 0xFF ? 0 : src_row[group.src_byte_base + idx];
        acc += group.weights[t][k] * byte_val;
      }
      expected[k][c] = CStretchEngine::PixelFromFixed(acc);
    }
  }
}

// Weights spanning the 17-bit fixed-point range the weight table produces:
// zero, one, the midpoint, and both ends.
constexpr std::array<uint32_t, 5> kTestWeights = {0, 1, 32768, 65535, 65536};

// Source bytes spanning uint8_t's range, plus two values that are not a
// power of two.
constexpr std::array<uint8_t, 8> kTestBytes = {0,   1,   84,  127,
                                               128, 171, 254, 255};

// Builds the `seed`-th of a small, fixed set of test groups with `taps` taps
// over a `wide` (32 byte) or narrow (16 byte) window at the start of the
// row: every (column, tap) gathers `gathered` consecutive bytes from a
// position and a weight chosen from kTestWeights, cycling both with a
// different phase per (column, tap) so consecutive seeds exercise different
// combinations -- deliberately, rather than hoping random search eventually
// covers the same positions and weights.
StretchHorzGroup BuildTestGroup(int seed, int gathered, uint32_t taps, bool wide) {
  const int load_bytes = wide ? 32 : 16;
  const int max_pos = load_bytes - gathered;
  const std::array<int, 3> positions = {0, max_pos / 2, max_pos};
  StretchHorzGroup group = {};
  group.taps = taps;
  group.wide = wide;
  std::ranges::fill(group.idx[0], 0xFF);
  std::ranges::fill(group.idx[1], 0xFF);
  int n = seed;
  for (int k = 0; k < 4; ++k) {
    for (uint32_t t = 0; t < taps; ++t) {
      const int pos = positions[n % positions.size()];
      for (int c = 0; c < gathered; ++c) {
        group.idx[t][k * 4 + c] = static_cast<uint8_t>(pos + c);
      }
      group.weights[t][k] = kTestWeights[n % kTestWeights.size()];
      ++n;
    }
  }
  return group;
}

// How many of BuildTestGroup()'s seeds to check: enough that, since
// kTestWeights and the 3 positions are coprime in count, every (position,
// weight) pair is reached at least once, shifted by a different phase per
// seed.
constexpr int kTestSeedCount = 16;

// A source row of 48 bytes cycling through kTestBytes (whole BGR and BGRA
// pixels) and a 24-byte destination prefilled with 0xAA, so untouched bytes
// can be told apart.
struct KernelBuffers {
  std::array<uint8_t, 48> src_row;
  std::array<uint8_t, 24> dest;

  KernelBuffers() {
    for (size_t i = 0; i < src_row.size(); ++i) {
      src_row[i] = kTestBytes[i % kTestBytes.size()];
    }
    std::ranges::fill(dest, 0xAA);
  }
};

// Runs one group through the kernel for the destination pixel type `Pixel`
// (a byte, FX_BGR_STRUCT or FX_BGRA_STRUCT).
template <typename Pixel>
void RunOneGroup(const StretchHorzGroup& group, KernelBuffers& buffers) {
  StretchHorzRunSimd(
      pdfium::span_from_ref(group),
      fxcrt::reinterpret_span<const Pixel>(
          pdfium::span<const uint8_t>(buffers.src_row)),
      fxcrt::reinterpret_span<Pixel>(pdfium::span(buffers.dest)));
}

template <int kBpp, typename Pixel>
void CheckTestGroups(uint32_t taps, bool wide) {
  const int gathered = kBpp == 1 ? 1 : 3;
  for (int seed = 0; seed < kTestSeedCount; ++seed) {
    KernelBuffers buffers;
    const StretchHorzGroup group = BuildTestGroup(seed, gathered, taps, wide);
    std::array<std::array<uint8_t, 3>, 4> expected = {};
    ScalarGroupReference(kBpp, group, buffers.src_row, expected);

    RunOneGroup<Pixel>(group, buffers);
    for (int k = 0; k < 4; ++k) {
      for (int c = 0; c < gathered; ++c) {
        EXPECT_EQ(expected[k][c], buffers.dest[k * kBpp + c])
            << "seed " << seed << " column " << k << " channel " << c;
      }
      if constexpr (kBpp == 4) {
        EXPECT_EQ(0xAA, buffers.dest[k * 4 + 3])
            << "seed " << seed << " column " << k;
      }
    }
    // The 4 columns of kBpp bytes are all the group may write.
    for (size_t i = 4 * kBpp; i < buffers.dest.size(); ++i) {
      EXPECT_EQ(0xAA, buffers.dest[i]) << "seed " << seed << " byte " << i;
    }
  }
}

// The alpha arm's per-column arithmetic (kManyBpptoManyBppWithAlpha in
// ContinueStretchHorz()), re-derived: each tap contributes weight *
// src.alpha / 255 (truncating) times the channel, the alpha channel
// accumulates that pixel weight itself, and the output alpha is bits 16..23
// of 255 times the accumulated pixel weight.
void ScalarAlphaReference(const StretchHorzGroup& group,
                          pdfium::span<const uint8_t> src_row,
                          std::array<std::array<uint8_t, 4>, 4>& expected) {
  for (int k = 0; k < 4; ++k) {
    std::array<uint32_t, 4> acc = {};
    for (uint32_t t = 0; t < group.taps; ++t) {
      std::array<uint8_t, 4> px;
      for (int c = 0; c < 4; ++c) {
        const uint8_t idx = group.idx[t][k * 4 + c];
        px[c] = idx == 0xFF ? 0 : src_row[group.src_byte_base + idx];
      }
      const uint32_t pixel_weight = group.weights[t][k] * px[3] / 255;
      for (int c = 0; c < 3; ++c) {
        acc[c] += pixel_weight * px[c];
      }
      acc[3] += pixel_weight;
    }
    for (int c = 0; c < 3; ++c) {
      expected[k][c] = CStretchEngine::PixelFromFixed(acc[c]);
    }
    expected[k][3] = CStretchEngine::PixelFromFixed(255 * acc[3]);
  }
}

void CheckTestAlphaGroups(uint32_t taps, bool wide) {
  for (int seed = 0; seed < kTestSeedCount; ++seed) {
    KernelBuffers buffers;
    // A 4-byte gather per column: the alpha arm reads all of B, G, R, A.
    const StretchHorzGroup group = BuildTestGroup(seed, 4, taps, wide);
    std::array<std::array<uint8_t, 4>, 4> expected = {};
    ScalarAlphaReference(group, buffers.src_row, expected);

    using Pixel = FX_BGRA_STRUCT<uint8_t>;
    StretchHorzRunAlphaSimd(
        pdfium::span_from_ref(group),
        fxcrt::reinterpret_span<const Pixel>(
            pdfium::span<const uint8_t>(buffers.src_row)),
        fxcrt::reinterpret_span<Pixel>(pdfium::span(buffers.dest)));
    for (int k = 0; k < 4; ++k) {
      for (int c = 0; c < 4; ++c) {
        EXPECT_EQ(expected[k][c], buffers.dest[k * 4 + c])
            << "seed " << seed << " column " << k << " channel " << c;
      }
    }
    for (size_t i = 16; i < buffers.dest.size(); ++i) {
      EXPECT_EQ(0xAA, buffers.dest[i]) << "seed " << seed << " byte " << i;
    }
  }
}

}  // namespace

TEST(CStretchEngine, HorzRunSimdMatchesScalarReference) {
  if (!StretchHorzSimdAvailable()) {
    GTEST_SKIP();
  }
  for (uint32_t taps : {1u, 2u}) {
    for (bool wide : {false, true}) {
      CheckTestGroups<1, uint8_t>(taps, wide);
      CheckTestGroups<3, FX_BGR_STRUCT<uint8_t>>(taps, wide);
      CheckTestGroups<4, FX_BGRA_STRUCT<uint8_t>>(taps, wide);
    }
  }
}

TEST(CStretchEngine, HorzRunAlphaSimdMatchesScalarReference) {
  if (!StretchHorzSimdAvailable()) {
    GTEST_SKIP();
  }
  for (uint32_t taps : {1u, 2u}) {
    for (bool wide : {false, true}) {
      CheckTestAlphaGroups(taps, wide);
    }
  }
}

// The run contract: every group in the span is processed, in order, each
// writing the next 4 destination pixels, and nothing beyond them. Every
// group here is one identity tap (weight 65536) over bytes 0..11 of the row,
// so each group's output must equal the row's first 12 bytes.
TEST(CStretchEngine, HorzRunSimdProcessesEveryGroup) {
  if (!StretchHorzSimdAvailable()) {
    GTEST_SKIP();
  }
  std::array<uint8_t, 48> src_row;
  for (size_t i = 0; i < src_row.size(); ++i) {
    src_row[i] = static_cast<uint8_t>(0x10 + i);
  }
  std::array<StretchHorzGroup, 3> groups = {};
  for (StretchHorzGroup& group : groups) {
    group.taps = 1;
    std::ranges::fill(group.idx[0], 0xFF);
    std::ranges::fill(group.idx[1], 0xFF);
    for (int k = 0; k < 4; ++k) {
      for (int c = 0; c < 3; ++c) {
        group.idx[0][k * 4 + c] = static_cast<uint8_t>(k * 3 + c);
      }
      group.weights[0][k] = CStretchEngine::kFixedPointOne;
    }
  }
  std::array<uint8_t, 48> dest;
  std::ranges::fill(dest, 0xAA);
  using Pixel = FX_BGR_STRUCT<uint8_t>;
  StretchHorzRunSimd(groups,
                     fxcrt::reinterpret_span<const Pixel>(
                         pdfium::span<const uint8_t>(src_row)),
                     fxcrt::reinterpret_span<Pixel>(pdfium::span(dest)));
  const pdfium::span<const uint8_t> dest_span(dest);
  const pdfium::span<const uint8_t> row_span(src_row);
  for (size_t j = 0; j < groups.size(); ++j) {
    EXPECT_EQ(row_span.first(12u), dest_span.subspan(12 * j, 12u))
        << "group " << j;
  }
  for (size_t i = 12 * groups.size(); i < dest.size(); ++i) {
    EXPECT_EQ(0xAA, dest[i]) << "byte " << i;
  }
}
