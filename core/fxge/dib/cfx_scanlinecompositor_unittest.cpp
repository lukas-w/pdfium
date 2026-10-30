// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_scanlinecompositor.h"

#include <stdint.h>

#include <array>
#include <vector>

#include "build/build_config.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/zip.h"
#include "core/fxge/dib/cfx_scanlinecompositor_hwy.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAreArray;

namespace {

constexpr FX_BGRA_STRUCT<uint8_t> kDestScan[] = {
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
};
constexpr FX_BGRA_STRUCT<uint8_t> kSrcScan1[] = {
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 0},
    {.blue = 255, .green = 100, .red = 0, .alpha = 255},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
    {.blue = 255, .green = 100, .red = 0, .alpha = 100},
    {.blue = 255, .green = 100, .red = 0, .alpha = 200},
};
constexpr FX_BGRA_STRUCT<uint8_t> kSrcScan2[] = {
    {.blue = 100, .green = 0, .red = 255, .alpha = 0},
    {.blue = 100, .green = 0, .red = 255, .alpha = 255},
    {.blue = 100, .green = 0, .red = 255, .alpha = 0},
    {.blue = 100, .green = 0, .red = 255, .alpha = 255},
    {.blue = 100, .green = 0, .red = 255, .alpha = 100},
    {.blue = 100, .green = 0, .red = 255, .alpha = 200},
    {.blue = 100, .green = 0, .red = 255, .alpha = 100},
    {.blue = 100, .green = 0, .red = 255, .alpha = 200},
};
constexpr FX_BGRA_STRUCT<uint8_t> kSrcScan3[] = {
    {.blue = 0, .green = 255, .red = 100, .alpha = 0},
    {.blue = 0, .green = 255, .red = 100, .alpha = 255},
    {.blue = 0, .green = 255, .red = 100, .alpha = 0},
    {.blue = 0, .green = 255, .red = 100, .alpha = 255},
    {.blue = 0, .green = 255, .red = 100, .alpha = 100},
    {.blue = 0, .green = 255, .red = 100, .alpha = 200},
    {.blue = 0, .green = 255, .red = 100, .alpha = 100},
    {.blue = 0, .green = 255, .red = 100, .alpha = 200},
};

void RunTest(CFX_ScanlineCompositor& compositor,
             pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
             pdfium::span<const FX_BGRA_STRUCT<uint8_t>> expectations) {
  std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
  fxcrt::Copy(kDestScan, dest_scan);
  compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                    pdfium::as_bytes(src_span),
                                    dest_scan.size(), {});
  EXPECT_THAT(dest_scan, ElementsAreArray(expectations));
}

#if defined(PDF_USE_SKIA)
void PreMultiplyScanLine(pdfium::span<FX_BGRA_STRUCT<uint8_t>> scanline) {
  for (auto& pixel : scanline) {
    pixel = PreMultiplyColor(pixel);
  }
}

void UnPreMultiplyScanLine(pdfium::span<FX_BGRA_STRUCT<uint8_t>> scanline) {
  for (auto& pixel : scanline) {
    pixel = UnPreMultiplyColor(pixel);
  }
}

void RunPreMultiplyTest(
    CFX_ScanlineCompositor& compositor,
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> src_span,
    pdfium::span<const FX_BGRA_STRUCT<uint8_t>> expectations) {
  std::array<FX_BGRA_STRUCT<uint8_t>, 8> dest_scan;
  fxcrt::Copy(kDestScan, dest_scan);
  PreMultiplyScanLine(dest_scan);
  std::array<FX_BGRA_STRUCT<uint8_t>, 8> src_scan;
  fxcrt::Copy(src_span, src_scan);
  PreMultiplyScanLine(src_scan);
  compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                    pdfium::as_byte_span(src_scan),
                                    dest_scan.size(), {});
  UnPreMultiplyScanLine(dest_scan);
  EXPECT_THAT(dest_scan, ElementsAreArray(expectations));
}
#endif  // defined(PDF_USE_SKIA)

// Independent re-derivation of CompositeRowBgra2BgraSimd's contract
// (AlphaUnion()/AlphaMerge(), fx_dib.h); not copied from the kernel.
FX_BGRA_STRUCT<uint8_t> RefBgra2Bgra(const FX_BGRA_STRUCT<uint8_t>& src,
                                     const FX_BGRA_STRUCT<uint8_t>& back) {
  if (back.alpha == 0) {
    return src;
  }
  if (src.alpha == 0) {
    return back;
  }
  const int dest_alpha = back.alpha + src.alpha - back.alpha * src.alpha / 255;
  const int ratio = src.alpha * 255 / dest_alpha;
  return {
      .blue = static_cast<uint8_t>(AlphaMerge(back.blue, src.blue, ratio)),
      .green = static_cast<uint8_t>(AlphaMerge(back.green, src.green, ratio)),
      .red = static_cast<uint8_t>(AlphaMerge(back.red, src.red, ratio)),
      .alpha = static_cast<uint8_t>(dest_alpha),
  };
}

void CheckBgra2Bgra(size_t width) {
  // alpha cycle; dest phase `(i / 4) % 4`
  static constexpr std::array<uint8_t, 4> kAlphaCycle = {0, 255, 100, 200};
  std::vector<FX_BGRA_STRUCT<uint8_t>> src(width);
  std::vector<FX_BGRA_STRUCT<uint8_t>> dest(width);
  for (size_t i = 0; i < width; ++i) {
    // per 32 px: [0,16) src a in {0,255} -> all-copy/keep fast path;
    // [16,32) mid a -> divide path, incl. mid-over-mid
    const uint8_t src_a = (i % 32 < 16)
                              ? static_cast<uint8_t>((i & 1) ? 255 : 0)
                              : kAlphaCycle[i % 4];
    src[i] = {.blue = static_cast<uint8_t>(i * 7 + 1),
              .green = static_cast<uint8_t>(i * 13 + 2),
              .red = static_cast<uint8_t>(i * 29 + 3),
              .alpha = src_a};
    dest[i] = {.blue = static_cast<uint8_t>(i * 31 + 5),
               .green = static_cast<uint8_t>(i * 17 + 7),
               .red = static_cast<uint8_t>(i * 11 + 9),
               .alpha = kAlphaCycle[(i / 4) % 4]};
  }
  std::vector<FX_BGRA_STRUCT<uint8_t>> expected(width);
  for (auto [input, back, want] : fxcrt::Zip(src, dest, expected)) {
    want = RefBgra2Bgra(input, back);
  }
  CompositeRowBgra2BgraSimd(src, dest);
  for (size_t i = 0; i < width; ++i) {
    EXPECT_EQ(expected[i], dest[i]) << "width " << width << " pixel " << i;
  }
}

void CheckBgra2BgrxForceOpaque(size_t width) {
  std::vector<FX_BGRA_STRUCT<uint8_t>> src(width);
  for (size_t i = 0; i < width; ++i) {
    src[i] = {.blue = static_cast<uint8_t>(i * 5 + 1),
              .green = static_cast<uint8_t>(i * 9 + 2),
              .red = static_cast<uint8_t>(i * 19 + 3),
              .alpha = static_cast<uint8_t>(i * 3 + 4)};  // src alpha ignored
  }
  // sentinel dest; expect src.rgb, alpha 0xFF
  std::vector<FX_BGRA_STRUCT<uint8_t>> dest(
      width, {.blue = 0xAA, .green = 0xAA, .red = 0xAA, .alpha = 0xAA});
  CompositeRowBgra2BgrxForceOpaqueSimd(src, dest);
  for (size_t i = 0; i < width; ++i) {
    const FX_BGRA_STRUCT<uint8_t> want = {.blue = src[i].blue,
                                          .green = src[i].green,
                                          .red = src[i].red,
                                          .alpha = 255};
    EXPECT_EQ(want, dest[i]) << "width " << width << " pixel " << i;
  }
}

}  // namespace

namespace fxge {

inline bool operator==(const FX_BGRA_STRUCT<uint8_t>& lhs,
                       const FX_BGRA_STRUCT<uint8_t>& rhs) {
  return lhs.blue == rhs.blue && lhs.green == rhs.green && lhs.red == rhs.red &&
         lhs.alpha == rhs.alpha;
}

}  // namespace fxge

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraNormal) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kNormal,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 161},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 158, .green = 38, .red = 158, .alpha = 161},
      {.blue = 115, .green = 10, .red = 229, .alpha = 222},
      {.blue = 185, .green = 55, .red = 114, .alpha = 222},
      {.blue = 127, .green = 18, .red = 209, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 97, .green = 196, .red = 61, .alpha = 161},
      {.blue = 26, .green = 239, .red = 89, .alpha = 222},
      {.blue = 141, .green = 169, .red = 44, .alpha = 222},
      {.blue = 46, .green = 227, .red = 81, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraScreen) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kScreen,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 161, .red = 0, .alpha = 255},
      {.blue = 255, .green = 114, .red = 0, .alpha = 161},
      {.blue = 255, .green = 120, .red = 0, .alpha = 222},
      {.blue = 255, .green = 121, .red = 0, .alpha = 222},
      {.blue = 255, .green = 138, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 255, .alpha = 255},
      {.blue = 196, .green = 62, .red = 158, .alpha = 161},
      {.blue = 169, .green = 45, .red = 229, .alpha = 222},
      {.blue = 239, .green = 90, .red = 114, .alpha = 222},
      {.blue = 227, .green = 81, .red = 209, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 255, .red = 100, .alpha = 255},
      {.blue = 158, .green = 196, .red = 61, .alpha = 161},
      {.blue = 115, .green = 239, .red = 89, .alpha = 222},
      {.blue = 230, .green = 169, .red = 44, .alpha = 222},
      {.blue = 209, .green = 227, .red = 81, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraDarken) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kDarken,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 161},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 100, .green = 0, .red = 0, .alpha = 255},
      {.blue = 158, .green = 38, .red = 96, .alpha = 161},
      {.blue = 115, .green = 10, .red = 139, .alpha = 222},
      {.blue = 185, .green = 55, .red = 24, .alpha = 222},
      {.blue = 127, .green = 18, .red = 45, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 100, .red = 0, .alpha = 255},
      {.blue = 97, .green = 158, .red = 37, .alpha = 161},
      {.blue = 26, .green = 184, .red = 53, .alpha = 222},
      {.blue = 141, .green = 114, .red = 9, .alpha = 222},
      {.blue = 46, .green = 127, .red = 17, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraSoftLight) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kSoftLight,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 87, .red = 0, .alpha = 255},
      {.blue = 255, .green = 96, .red = 0, .alpha = 161},
      {.blue = 255, .green = 94, .red = 0, .alpha = 222},
      {.blue = 255, .green = 95, .red = 0, .alpha = 222},
      {.blue = 255, .green = 90, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 40, .red = 0, .alpha = 255},
      {.blue = 196, .green = 47, .red = 96, .alpha = 161},
      {.blue = 169, .green = 23, .red = 139, .alpha = 222},
      {.blue = 239, .green = 69, .red = 24, .alpha = 222},
      {.blue = 227, .green = 43, .red = 45, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 160, .red = 0, .alpha = 255},
      {.blue = 158, .green = 172, .red = 37, .alpha = 161},
      {.blue = 115, .green = 205, .red = 53, .alpha = 222},
      {.blue = 230, .green = 135, .red = 9, .alpha = 222},
      {.blue = 209, .green = 165, .red = 17, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraHue) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kHue,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 161},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 158, .green = 38, .red = 158, .alpha = 161},
      {.blue = 115, .green = 10, .red = 229, .alpha = 222},
      {.blue = 185, .green = 55, .red = 114, .alpha = 222},
      {.blue = 127, .green = 18, .red = 209, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 123, .red = 49, .alpha = 255},
      {.blue = 97, .green = 163, .red = 49, .alpha = 161},
      {.blue = 26, .green = 192, .red = 71, .alpha = 222},
      {.blue = 141, .green = 122, .red = 26, .alpha = 222},
      {.blue = 46, .green = 141, .red = 49, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraSaturation) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kSaturation,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 161},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 196, .green = 62, .red = 96, .alpha = 161},
      {.blue = 169, .green = 45, .red = 139, .alpha = 222},
      {.blue = 239, .green = 90, .red = 24, .alpha = 222},
      {.blue = 227, .green = 81, .red = 45, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 158, .green = 158, .red = 37, .alpha = 161},
      {.blue = 115, .green = 184, .red = 53, .alpha = 222},
      {.blue = 230, .green = 114, .red = 9, .alpha = 222},
      {.blue = 209, .green = 127, .red = 17, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraColor) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kColor,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 161},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 158, .green = 38, .red = 158, .alpha = 161},
      {.blue = 115, .green = 10, .red = 229, .alpha = 222},
      {.blue = 185, .green = 55, .red = 114, .alpha = 222},
      {.blue = 127, .green = 18, .red = 209, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 123, .red = 49, .alpha = 255},
      {.blue = 97, .green = 163, .red = 49, .alpha = 161},
      {.blue = 26, .green = 192, .red = 71, .alpha = 222},
      {.blue = 141, .green = 122, .red = 26, .alpha = 222},
      {.blue = 46, .green = 141, .red = 49, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraLuminosity) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                              /*src_format=*/FXDIB_Format::kBgra,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kLuminosity,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 255, .green = 100, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 161},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 222},
      {.blue = 255, .green = 100, .red = 0, .alpha = 244},
  };
  RunTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 100, .green = 0, .red = 255, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 196, .green = 62, .red = 96, .alpha = 161},
      {.blue = 169, .green = 45, .red = 139, .alpha = 222},
      {.blue = 239, .green = 90, .red = 24, .alpha = 222},
      {.blue = 227, .green = 81, .red = 45, .alpha = 244},
  };
  RunTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 255, .red = 100, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 185, .red = 142, .alpha = 255},
      {.blue = 158, .green = 178, .red = 71, .alpha = 161},
      {.blue = 115, .green = 214, .red = 104, .alpha = 222},
      {.blue = 230, .green = 144, .red = 59, .alpha = 222},
      {.blue = 209, .green = 181, .red = 108, .alpha = 244},
  };
  RunTest(compositor, kSrcScan3, kExpectations3);
}

#if defined(PDF_USE_SKIA)
TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulNormal) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kNormal,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
#if BUILDFLAG(IS_APPLE) && defined(ARCH_CPU_ARM64)
      {.blue = 255, .green = 99, .red = 0, .alpha = 161},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
#else
      {.blue = 255, .green = 98, .red = 0, .alpha = 160},
      {.blue = 255, .green = 99, .red = 0, .alpha = 221},
      {.blue = 255, .green = 99, .red = 0, .alpha = 221},
#endif
      {.blue = 255, .green = 99, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
#if BUILDFLAG(IS_APPLE) && defined(ARCH_CPU_ARM64)
      {.blue = 158, .green = 38, .red = 158, .alpha = 161},
      {.blue = 114, .green = 9, .red = 229, .alpha = 222},
      {.blue = 184, .green = 53, .red = 114, .alpha = 222},
#else
      {.blue = 157, .green = 36, .red = 159, .alpha = 160},
      {.blue = 114, .green = 9, .red = 230, .alpha = 221},
      {.blue = 184, .green = 54, .red = 115, .alpha = 221},
#endif
      {.blue = 126, .green = 17, .red = 209, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
#if BUILDFLAG(IS_APPLE) && defined(ARCH_CPU_ARM64)
      {.blue = 96, .green = 196, .red = 61, .alpha = 161},
      {.blue = 25, .green = 238, .red = 89, .alpha = 222},
      {.blue = 140, .green = 168, .red = 44, .alpha = 222},
#else
      {.blue = 95, .green = 196, .red = 62, .alpha = 160},
      {.blue = 24, .green = 240, .red = 90, .alpha = 221},
      {.blue = 139, .green = 169, .red = 45, .alpha = 221},
#endif
      {.blue = 45, .green = 227, .red = 81, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulScreen) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kScreen,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 161, .red = 0, .alpha = 255},
      {.blue = 255, .green = 114, .red = 0, .alpha = 161},
      {.blue = 255, .green = 120, .red = 0, .alpha = 222},
      {.blue = 255, .green = 120, .red = 0, .alpha = 222},
      {.blue = 255, .green = 138, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 255, .alpha = 255},
      {.blue = 196, .green = 61, .red = 158, .alpha = 161},
      {.blue = 168, .green = 44, .red = 229, .alpha = 222},
      {.blue = 238, .green = 89, .red = 114, .alpha = 222},
      {.blue = 227, .green = 81, .red = 209, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 255, .red = 100, .alpha = 255},
      {.blue = 158, .green = 196, .red = 61, .alpha = 161},
      {.blue = 114, .green = 238, .red = 89, .alpha = 222},
      {.blue = 229, .green = 168, .red = 44, .alpha = 222},
      {.blue = 209, .green = 227, .red = 81, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulDarken) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kDarken,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
#if BUILDFLAG(IS_APPLE) && defined(ARCH_CPU_ARM64)
      {.blue = 255, .green = 99, .red = 0, .alpha = 161},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
#else
      {.blue = 253, .green = 98, .red = 0, .alpha = 161},
      {.blue = 253, .green = 98, .red = 0, .alpha = 222},
      {.blue = 253, .green = 98, .red = 0, .alpha = 222},
#endif
      {.blue = 255, .green = 99, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 100, .green = 0, .red = 0, .alpha = 255},
#if BUILDFLAG(IS_APPLE) && defined(ARCH_CPU_ARM64)
      {.blue = 158, .green = 38, .red = 96, .alpha = 161},
      {.blue = 114, .green = 9, .red = 140, .alpha = 222},
      {.blue = 184, .green = 53, .red = 25, .alpha = 222},
#else
      {.blue = 156, .green = 36, .red = 95, .alpha = 161},
      {.blue = 113, .green = 9, .red = 138, .alpha = 222},
      {.blue = 183, .green = 53, .red = 24, .alpha = 222},
#endif
      {.blue = 126, .green = 17, .red = 45, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 100, .red = 0, .alpha = 255},
#if BUILDFLAG(IS_APPLE) && defined(ARCH_CPU_ARM64)
      {.blue = 96, .green = 158, .red = 38, .alpha = 161},
      {.blue = 25, .green = 184, .red = 53, .alpha = 222},
      {.blue = 140, .green = 114, .red = 9, .alpha = 222},
#else
      {.blue = 95, .green = 156, .red = 36, .alpha = 161},
      {.blue = 24, .green = 183, .red = 53, .alpha = 222},
      {.blue = 138, .green = 113, .red = 9, .alpha = 222},
#endif
      {.blue = 45, .green = 126, .red = 17, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulSoftLight) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kSoftLight,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 87, .red = 0, .alpha = 255},
      {.blue = 255, .green = 96, .red = 0, .alpha = 161},
      {.blue = 255, .green = 94, .red = 0, .alpha = 222},
      {.blue = 255, .green = 94, .red = 0, .alpha = 222},
      {.blue = 255, .green = 91, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 39, .red = 0, .alpha = 255},
      {.blue = 196, .green = 47, .red = 96, .alpha = 161},
      {.blue = 168, .green = 22, .red = 140, .alpha = 222},
      {.blue = 238, .green = 67, .red = 25, .alpha = 222},
      {.blue = 227, .green = 43, .red = 45, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 160, .red = 0, .alpha = 255},
      {.blue = 158, .green = 172, .red = 38, .alpha = 161},
      {.blue = 114, .green = 205, .red = 53, .alpha = 222},
      {.blue = 229, .green = 135, .red = 9, .alpha = 222},
      {.blue = 209, .green = 165, .red = 17, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulHue) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kHue,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 99, .red = 0, .alpha = 161},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 99, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 99, .green = 0, .red = 254, .alpha = 255},
      {.blue = 158, .green = 38, .red = 158, .alpha = 161},
      {.blue = 113, .green = 9, .red = 228, .alpha = 222},
      {.blue = 183, .green = 53, .red = 113, .alpha = 222},
      {.blue = 126, .green = 17, .red = 208, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 123, .red = 48, .alpha = 255},
      {.blue = 96, .green = 163, .red = 49, .alpha = 161},
      {.blue = 25, .green = 192, .red = 71, .alpha = 222},
      {.blue = 140, .green = 122, .red = 26, .alpha = 222},
      {.blue = 45, .green = 141, .red = 48, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulSaturation) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kSaturation,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 99, .red = 0, .alpha = 161},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 99, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 196, .green = 61, .red = 96, .alpha = 161},
      {.blue = 168, .green = 44, .red = 140, .alpha = 222},
      {.blue = 238, .green = 89, .red = 25, .alpha = 222},
      {.blue = 227, .green = 81, .red = 45, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 158, .green = 158, .red = 38, .alpha = 161},
      {.blue = 114, .green = 184, .red = 53, .alpha = 222},
      {.blue = 229, .green = 114, .red = 9, .alpha = 222},
      {.blue = 209, .green = 126, .red = 17, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulColor) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kColor,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 99, .red = 0, .alpha = 161},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 99, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 99, .green = 0, .red = 254, .alpha = 255},
      {.blue = 158, .green = 38, .red = 158, .alpha = 161},
      {.blue = 113, .green = 9, .red = 228, .alpha = 222},
      {.blue = 183, .green = 53, .red = 113, .alpha = 222},
      {.blue = 126, .green = 17, .red = 208, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 0, .green = 123, .red = 48, .alpha = 255},
      {.blue = 96, .green = 163, .red = 49, .alpha = 161},
      {.blue = 25, .green = 192, .red = 71, .alpha = 222},
      {.blue = 140, .green = 122, .red = 26, .alpha = 222},
      {.blue = 45, .green = 141, .red = 48, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgraPremulLuminosity) {
  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgraPremul,
                              /*src_format=*/FXDIB_Format::kBgraPremul,
                              /*src_palette=*/{},
                              /*mask_color=*/0,
                              /*blend_type=*/BlendMode::kLuminosity,
                              /*bRgbByteOrder=*/false));

  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations1[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 99, .red = 0, .alpha = 161},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 98, .red = 0, .alpha = 222},
      {.blue = 255, .green = 99, .red = 0, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan1, kExpectations1);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations2[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 100, .green = 0, .red = 255, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 100, .red = 1, .alpha = 255},
      {.blue = 196, .green = 61, .red = 96, .alpha = 161},
      {.blue = 168, .green = 44, .red = 140, .alpha = 222},
      {.blue = 238, .green = 89, .red = 25, .alpha = 222},
      {.blue = 227, .green = 81, .red = 46, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan2, kExpectations2);
  static constexpr FX_BGRA_STRUCT<uint8_t> kExpectations3[] = {
      {.blue = 0, .green = 0, .red = 0, .alpha = 0},
      {.blue = 0, .green = 255, .red = 100, .alpha = 255},
      {.blue = 255, .green = 100, .red = 0, .alpha = 255},
      {.blue = 255, .green = 186, .red = 142, .alpha = 255},
      {.blue = 158, .green = 178, .red = 72, .alpha = 161},
      {.blue = 114, .green = 214, .red = 104, .alpha = 222},
      {.blue = 229, .green = 144, .red = 59, .alpha = 222},
      {.blue = 209, .green = 182, .red = 109, .alpha = 243},
  };
  RunPreMultiplyTest(compositor, kSrcScan3, kExpectations3);
}
#endif  // defined(PDF_USE_SKIA)

TEST(ScanlineCompositorTest, CompositeRgbBitmapLineBgrToBgraNormal) {
  static constexpr FX_BGR_STRUCT<uint8_t> kBgrSrc[] = {
      {.blue = 10, .green = 20, .red = 30},
      {.blue = 40, .green = 50, .red = 60},
      {.blue = 70, .green = 80, .red = 90},
      {.blue = 100, .green = 110, .red = 120},
      {.blue = 130, .green = 140, .red = 150},
  };

  {
    CFX_ScanlineCompositor compositor;
    ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                                /*src_format=*/FXDIB_Format::kBgr,
                                /*src_palette=*/{},
                                /*mask_color=*/0,
                                /*blend_type=*/BlendMode::kNormal,
                                /*bRgbByteOrder=*/false));
    std::array<FX_BGRA_STRUCT<uint8_t>, 5> dest_scan = {};
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kBgrSrc),
                                      dest_scan.size(), {});
    static constexpr FX_BGRA_STRUCT<uint8_t> kExpectedBgra[] = {
        {.blue = 10, .green = 20, .red = 30, .alpha = 255},
        {.blue = 40, .green = 50, .red = 60, .alpha = 255},
        {.blue = 70, .green = 80, .red = 90, .alpha = 255},
        {.blue = 100, .green = 110, .red = 120, .alpha = 255},
        {.blue = 130, .green = 140, .red = 150, .alpha = 255},
    };
    EXPECT_THAT(dest_scan, ElementsAreArray(kExpectedBgra));
  }

  {
    CFX_ScanlineCompositor compositor;
    ASSERT_TRUE(compositor.Init(/*dest_format=*/FXDIB_Format::kBgra,
                                /*src_format=*/FXDIB_Format::kBgr,
                                /*src_palette=*/{},
                                /*mask_color=*/0,
                                /*blend_type=*/BlendMode::kNormal,
                                /*bRgbByteOrder=*/true));
    std::array<FX_BGRA_STRUCT<uint8_t>, 5> dest_scan = {};
    compositor.CompositeRgbBitmapLine(pdfium::as_writable_byte_span(dest_scan),
                                      pdfium::as_byte_span(kBgrSrc),
                                      dest_scan.size(), {});
    static constexpr FX_BGRA_STRUCT<uint8_t> kExpectedRgba[] = {
        {.blue = 30, .green = 20, .red = 10, .alpha = 255},
        {.blue = 60, .green = 50, .red = 40, .alpha = 255},
        {.blue = 90, .green = 80, .red = 70, .alpha = 255},
        {.blue = 120, .green = 110, .red = 100, .alpha = 255},
        {.blue = 150, .green = 140, .red = 130, .alpha = 255},
    };
    EXPECT_THAT(dest_scan, ElementsAreArray(kExpectedRgba));
  }
}

// Vector body + sub-vector tail across widths spanning the SSE4/AVX2/AVX-512
// lane counts (4/8/16).
TEST(ScanlineCompositorSimdTest, Bgra2BgraMatchesScalar) {
  for (size_t width : {1u,  2u,  3u,  4u,  5u,  6u,  7u,  8u,  9u,   15u,
                       16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u, 100u, 257u}) {
    CheckBgra2Bgra(width);
  }
}

// Opaque copy: rgb preserved, alpha forced 0xFF, across vector/tail widths.
TEST(ScanlineCompositorSimdTest, Bgra2BgrxForceOpaqueMatchesScalar) {
  for (size_t width : {1u,  2u,  3u,  4u,  5u,  6u,  7u,  8u,  9u,   15u,
                       16u, 17u, 31u, 32u, 33u, 63u, 64u, 65u, 100u, 257u}) {
    CheckBgra2BgrxForceOpaque(width);
  }
}

// BGR24 -> BGRx expand via public path. RGB copied verbatim; padding byte
// 0xFF under libyuv, untouched by scalar (kBgrx padding, not displayed).
TEST(ScanlineCompositorSimdTest, CompositeRgbBitmapLineBgr24ToBgrx) {
  constexpr int kWidth = 67;  // non-multiple-of-16: exercises libyuv tail

  std::vector<uint8_t> src(static_cast<size_t>(kWidth) * 3);
  for (size_t i = 0; i < src.size(); ++i) {
    src[i] = static_cast<uint8_t>(i * 7 + 1);
  }
  constexpr uint8_t kPad = 0x3C;  // sentinel: 0xFF-stamp vs left-as-is
  std::vector<uint8_t> dest(static_cast<size_t>(kWidth) * 4, kPad);

  CFX_ScanlineCompositor compositor;
  ASSERT_TRUE(compositor.Init(FXDIB_Format::kBgrx, FXDIB_Format::kBgr,
                              /*src_palette=*/{}, /*mask_color=*/0,
                              BlendMode::kNormal, /*bRgbByteOrder=*/false));
  compositor.CompositeRgbBitmapLine(dest, src, kWidth, /*clip_scan=*/{});

  for (int i = 0; i < kWidth; ++i) {
    EXPECT_EQ(dest[i * 4 + 0], src[i * 3 + 0]) << "blue @ " << i;
    EXPECT_EQ(dest[i * 4 + 1], src[i * 3 + 1]) << "green @ " << i;
    EXPECT_EQ(dest[i * 4 + 2], src[i * 3 + 2]) << "red @ " << i;
#if defined(PDF_USE_LIBYUV)
    EXPECT_EQ(dest[i * 4 + 3], 0xFF) << "padding @ " << i;  // libyuv 0xFF
#else
    EXPECT_EQ(dest[i * 4 + 3], kPad) << "padding @ " << i;  // scalar leaves it
#endif
  }
}
