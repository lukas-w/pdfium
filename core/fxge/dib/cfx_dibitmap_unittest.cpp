// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_dibitmap.h"

#include <stdint.h>

#include <array>
#include <vector>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap_hwy.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using ::testing::ElementsAre;

// Boundary widths spanning the SSE4/AVX2/AVX-512 lane counts (4/8/16) plus
// their one-off neighbours, the empty row, and a long run.
constexpr auto kSimdWidths =
    std::to_array<size_t>({0,  1,  2,  3,  4,  5,  6,  7,  8,   9,  15,
                           16, 17, 31, 32, 33, 63, 64, 65, 100, 257});

void ExpectBgraEq(const FX_BGRA_STRUCT<uint8_t>& expected,
                  const FX_BGRA_STRUCT<uint8_t>& actual,
                  size_t width,
                  size_t i) {
  EXPECT_EQ(expected.blue, actual.blue) << "width " << width << " pixel " << i;
  EXPECT_EQ(expected.green, actual.green)
      << "width " << width << " pixel " << i;
  EXPECT_EQ(expected.red, actual.red) << "width " << width << " pixel " << i;
  EXPECT_EQ(expected.alpha, actual.alpha)
      << "width " << width << " pixel " << i;
}

std::vector<FX_BGRA_STRUCT<uint8_t>> MakeBgraRow(size_t width) {
  std::vector<FX_BGRA_STRUCT<uint8_t>> row(width);
  for (size_t i = 0; i < width; ++i) {
    row[i] = {.blue = static_cast<uint8_t>(i * 7 + 1),
              .green = static_cast<uint8_t>(i * 13 + 2),
              .red = static_cast<uint8_t>(i * 29 + 3),
              .alpha = static_cast<uint8_t>(i)};  // cycles all 256 alphas
  }
  return row;
}

// 64 + 3: several vector iterations and a 3-pixel tail on every target.
constexpr int kApiWidth = 67;
constexpr int kApiHeight = 2;

FX_BGRA_STRUCT<uint8_t> MakeApiPixel(int row, int i) {
  return {.blue = static_cast<uint8_t>(10 + i + row),
          .green = static_cast<uint8_t>(40 + i + row),
          .red = static_cast<uint8_t>(70 + i + row),
          .alpha = static_cast<uint8_t>(255 - 15 * i - 5 * row)};
}

// Includes 0 and 255.
uint8_t MakeApiMask(int row, int i) {
  return static_cast<uint8_t>(17 * i + 3 * row);
}

}  // namespace

TEST(CFXDIBitmapTest, Create) {
  auto pBitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_FALSE(pBitmap->Create(400, 300, FXDIB_Format::kInvalid));

  pBitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_TRUE(pBitmap->Create(400, 300, FXDIB_Format::k1bppRgb));
}

TEST(CFXDIBitmapTest, CalculatePitchAndSizeGood) {
  // Simple case with no provided pitch.
  std::optional<CFX_DIBitmap::PitchAndSize> result =
      CFX_DIBitmap::CalculatePitchAndSize(100, 200, FXDIB_Format::kBgra, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(400u, result.value().pitch);
  EXPECT_EQ(80000u, result.value().size);

  // Simple case with no provided pitch and different format.
  result =
      CFX_DIBitmap::CalculatePitchAndSize(100, 200, FXDIB_Format::k8bppRgb, 0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(100u, result.value().pitch);
  EXPECT_EQ(20000u, result.value().size);

  // Simple case with provided pitch matching width * bpp.
  result =
      CFX_DIBitmap::CalculatePitchAndSize(100, 200, FXDIB_Format::kBgra, 400);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(400u, result.value().pitch);
  EXPECT_EQ(80000u, result.value().size);

  // Simple case with provided pitch, where pitch exceeds width * bpp.
  result =
      CFX_DIBitmap::CalculatePitchAndSize(100, 200, FXDIB_Format::kBgra, 455);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(455u, result.value().pitch);
  EXPECT_EQ(91000u, result.value().size);
}

TEST(CFXDIBitmapTest, CalculatePitchAndSizeBad) {
  // Bad width / height.
  static const CFX_Size kBadDimensions[] = {
      {0, 0},   {-1, -1}, {-1, 0},   {0, -1},
      {0, 200}, {100, 0}, {-1, 200}, {100, -1},
  };
  for (const auto& dimension : kBadDimensions) {
    EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(
        dimension.width, dimension.height, FXDIB_Format::kBgra, 0));
    EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(
        dimension.width, dimension.height, FXDIB_Format::kBgra, 1));
  }

  // Bad format.
  EXPECT_FALSE(
      CFX_DIBitmap::CalculatePitchAndSize(100, 200, FXDIB_Format::kInvalid, 0));
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(
      100, 200, FXDIB_Format::kInvalid, 800));

  // Width too wide for claimed pitch.
  EXPECT_FALSE(
      CFX_DIBitmap::CalculatePitchAndSize(101, 200, FXDIB_Format::kBgra, 400));

  // Overflow cases with calculated pitch.
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(1073747000, 1,
                                                   FXDIB_Format::kBgra, 0));
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(1048576, 1024,
                                                   FXDIB_Format::kBgra, 0));
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(4194304, 1024,
                                                   FXDIB_Format::k8bppRgb, 0));

  // Overflow cases with provided pitch.
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(
      2147484000u, 1, FXDIB_Format::kBgra, 2147484000u));
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(
      1048576, 1024, FXDIB_Format::kBgra, 4194304));
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(
      4194304, 1024, FXDIB_Format::k8bppRgb, 4194304));
}

TEST(CFXDIBitmapTest, CalculatePitchAndSizeBoundary) {
  // Test boundary condition for pitch overflow.
  std::optional<CFX_DIBitmap::PitchAndSize> result =
      CFX_DIBitmap::CalculatePitchAndSize(536870911, 4, FXDIB_Format::k8bppRgb,
                                          0);
  ASSERT_TRUE(result);
  EXPECT_EQ(536870912u, result.value().pitch);
  EXPECT_EQ(2147483648u, result.value().size);
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(536870912, 4,
                                                   FXDIB_Format::k8bppRgb, 0));

  // Test boundary condition for size overflow.
  result = CFX_DIBitmap::CalculatePitchAndSize(68174084, 63,
                                               FXDIB_Format::k8bppRgb, 0);
  ASSERT_TRUE(result);
  EXPECT_EQ(68174084u, result.value().pitch);
  EXPECT_EQ(4294967292u, result.value().size);
  EXPECT_FALSE(CFX_DIBitmap::CalculatePitchAndSize(68174085, 63,
                                                   FXDIB_Format::k8bppRgb, 0));
}

TEST(CFXDIBitmapTest, GetScanlineAsWith24Bpp) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(3, 3, FXDIB_Format::kBgr));
  EXPECT_EQ(3, bitmap->GetWidth());
  EXPECT_EQ(12u, bitmap->GetPitch());

  EXPECT_EQ(36u, bitmap->GetBuffer().size());
  EXPECT_EQ(12u, bitmap->GetScanline(0).size());
  EXPECT_EQ(3u, bitmap->GetScanlineAs<FX_BGR_STRUCT<uint8_t>>(0).size());

  EXPECT_EQ(36u, bitmap->GetWritableBuffer().size());
  EXPECT_EQ(12u, bitmap->GetWritableScanline(0).size());
  EXPECT_EQ(3u,
            bitmap->GetWritableScanlineAs<FX_BGR_STRUCT<uint8_t>>(0).size());
}

#if defined(PDF_USE_SKIA)
TEST(CFXDIBitmapTest, UnPreMultiplyFromPreMultiplied) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(1, 1, FXDIB_Format::kBgraPremul));
  EXPECT_TRUE(bitmap->IsPremultiplied());
  FXARGB_SetDIB(bitmap->GetWritableBuffer().first<4u>(), 0x7f'7f'7f'7f);

  bitmap->UnPreMultiply();
  EXPECT_FALSE(bitmap->IsPremultiplied());
  EXPECT_THAT(bitmap->GetBuffer(), ElementsAre(0xff, 0xff, 0xff, 0x7f));
}

TEST(CFXDIBitmapTest, UnPreMultiplyFromUnPreMultiplied) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(1, 1, FXDIB_Format::kBgra));
  EXPECT_FALSE(bitmap->IsPremultiplied());
  FXARGB_SetDIB(bitmap->GetWritableBuffer().first<4u>(), 0x7f'ff'ff'ff);

  bitmap->UnPreMultiply();
  EXPECT_FALSE(bitmap->IsPremultiplied());
  EXPECT_THAT(bitmap->GetBuffer(), ElementsAre(0xff, 0xff, 0xff, 0x7f));
}

TEST(CFXDIBitmapTest, PreMultiplyFromUnPreMultiplied) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(1, 1, FXDIB_Format::kBgra));
  EXPECT_FALSE(bitmap->IsPremultiplied());
  FXARGB_SetDIB(bitmap->GetWritableBuffer().first<4u>(), 0x7f'ff'ff'ff);

  bitmap->PreMultiply();
  EXPECT_TRUE(bitmap->IsPremultiplied());
  EXPECT_THAT(bitmap->GetBuffer(), ElementsAre(0x7f, 0x7f, 0x7f, 0x7f));
}

TEST(CFXDIBitmapTest, PreMultiplyFromPreMultiplied) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(1, 1, FXDIB_Format::kBgraPremul));
  EXPECT_TRUE(bitmap->IsPremultiplied());
  FXARGB_SetDIB(bitmap->GetWritableBuffer().first<4u>(), 0x7f'7f'7f'7f);

  bitmap->PreMultiply();
  EXPECT_TRUE(bitmap->IsPremultiplied());
  EXPECT_THAT(bitmap->GetBuffer(), ElementsAre(0x7f, 0x7f, 0x7f, 0x7f));
}
#endif  // defined(PDF_USE_SKIA)

TEST(CFXDIBitmapTest, ConvertFormatBgrToBgra) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 1, FXDIB_Format::kBgr));
  auto bgr_row = bitmap->GetWritableScanlineAs<FX_BGR_STRUCT<uint8_t>>(0);
  bgr_row[0] = {11, 22, 33};
  bgr_row[1] = {44, 55, 66};

  ASSERT_TRUE(bitmap->ConvertFormat(FXDIB_Format::kBgra));
  ASSERT_EQ(2, bitmap->GetWidth());
  ASSERT_EQ(1, bitmap->GetHeight());
  EXPECT_EQ(FXDIB_Format::kBgra, bitmap->GetFormat());
  auto bgra_row = bitmap->GetScanlineAs<FX_BGRA_STRUCT<uint8_t>>(0);
  EXPECT_EQ(11u, bgra_row[0].blue);
  EXPECT_EQ(22u, bgra_row[0].green);
  EXPECT_EQ(33u, bgra_row[0].red);
  EXPECT_EQ(255u, bgra_row[0].alpha);
  EXPECT_EQ(44u, bgra_row[1].blue);
  EXPECT_EQ(55u, bgra_row[1].green);
  EXPECT_EQ(66u, bgra_row[1].red);
  EXPECT_EQ(255u, bgra_row[1].alpha);
}

TEST(CFXDIBitmapTest, MultiplyAlphaMaskFromBgrx) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 1, FXDIB_Format::kBgrx));
  auto row = bitmap->GetWritableScanlineAs<FX_BGRA_STRUCT<uint8_t>>(0);
  row[0] = {10, 20, 30, 0};
  row[1] = {40, 50, 60, 0};

  auto mask = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(mask->Create(2, 1, FXDIB_Format::k8bppMask));
  auto mask_row = mask->GetWritableScanline(0);
  mask_row[0] = 0x12;
  mask_row[1] = 0x34;

  ASSERT_TRUE(bitmap->MultiplyAlphaMask(mask));
  ASSERT_EQ(2, bitmap->GetWidth());
  ASSERT_EQ(1, bitmap->GetHeight());
  EXPECT_EQ(FXDIB_Format::kBgra, bitmap->GetFormat());
  auto out_row = bitmap->GetScanlineAs<FX_BGRA_STRUCT<uint8_t>>(0);
  EXPECT_EQ(10u, out_row[0].blue);
  EXPECT_EQ(20u, out_row[0].green);
  EXPECT_EQ(30u, out_row[0].red);
  EXPECT_EQ(0x12u, out_row[0].alpha);
  EXPECT_EQ(40u, out_row[1].blue);
  EXPECT_EQ(50u, out_row[1].green);
  EXPECT_EQ(60u, out_row[1].red);
  EXPECT_EQ(0x34u, out_row[1].alpha);
}

// Vector body + sub-vector tail: SIMD matches the scalar alpha * f / 255
// across lane-count boundaries and every constant alpha factor.
TEST(CFXDIBitmapTest, MultiplyAlphaSimdMatchesScalar) {
  for (size_t width : kSimdWidths) {
    const std::vector<FX_BGRA_STRUCT<uint8_t>> src = MakeBgraRow(width);
    std::vector<FX_BGRA_STRUCT<uint8_t>> work;
    for (int factor = 0; factor <= 255; ++factor) {
      work = src;
      MultiplyAlphaSimd(work, static_cast<uint8_t>(factor));
      for (size_t i = 0; i < width; ++i) {
        FX_BGRA_STRUCT<uint8_t> expected = src[i];
        expected.alpha = static_cast<uint8_t>(src[i].alpha * factor / 255);
        ExpectBgraEq(expected, work[i], width, i);
      }
    }
  }
}

// Same, for the per-pixel mask variant alpha * mask / 255. Mask bytes vary
// across pixels, and the shift sweeps every mask value over each alpha.
TEST(CFXDIBitmapTest, MultiplyAlphaMaskSimdMatchesScalar) {
  for (size_t width : kSimdWidths) {
    const std::vector<FX_BGRA_STRUCT<uint8_t>> src = MakeBgraRow(width);
    std::vector<uint8_t> mask(width);
    std::vector<FX_BGRA_STRUCT<uint8_t>> work;
    for (size_t shift = 0; shift < 256; ++shift) {
      for (size_t i = 0; i < width; ++i) {
        mask[i] = static_cast<uint8_t>(i * 37 + 11 + shift);
      }
      work = src;
      MultiplyAlphaMaskSimd(work, mask);
      for (size_t i = 0; i < width; ++i) {
        FX_BGRA_STRUCT<uint8_t> expected = src[i];
        expected.alpha = static_cast<uint8_t>(src[i].alpha * mask[i] / 255);
        ExpectBgraEq(expected, work[i], width, i);
      }
    }
  }
}

// Public API, multi-row kBgra: row loop, float -> uint8_t factor, BGR kept.
TEST(CFXDIBitmapTest, MultiplyAlphaOnBgra) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(kApiWidth, kApiHeight, FXDIB_Format::kBgra));
  for (int row = 0; row < kApiHeight; ++row) {
    auto dest = bitmap->GetWritableScanlineAs<FX_BGRA_STRUCT<uint8_t>>(row);
    for (int i = 0; i < kApiWidth; ++i) {
      dest[i] = MakeApiPixel(row, i);
    }
  }

  constexpr uint8_t kBitmapAlpha = 127;  // uint8_t(0.5f * 255.0f)
  ASSERT_TRUE(bitmap->MultiplyAlpha(0.5f));

  EXPECT_EQ(FXDIB_Format::kBgra, bitmap->GetFormat());
  for (int row = 0; row < kApiHeight; ++row) {
    auto out_row = bitmap->GetScanlineAs<FX_BGRA_STRUCT<uint8_t>>(row);
    for (int i = 0; i < kApiWidth; ++i) {
      FX_BGRA_STRUCT<uint8_t> expected = MakeApiPixel(row, i);
      expected.alpha =
          static_cast<uint8_t>(expected.alpha * kBitmapAlpha / 255);
      ExpectBgraEq(expected, out_row[i], kApiWidth, i);
    }
  }
}

// Public API, multi-row kBgra: alpha * mask, unlike kBgrx (alpha = mask).
TEST(CFXDIBitmapTest, MultiplyAlphaMaskOnBgra) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(kApiWidth, kApiHeight, FXDIB_Format::kBgra));
  auto mask = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(mask->Create(kApiWidth, kApiHeight, FXDIB_Format::k8bppMask));
  for (int row = 0; row < kApiHeight; ++row) {
    auto dest = bitmap->GetWritableScanlineAs<FX_BGRA_STRUCT<uint8_t>>(row);
    auto mask_row = mask->GetWritableScanline(row);
    for (int i = 0; i < kApiWidth; ++i) {
      dest[i] = MakeApiPixel(row, i);
      mask_row[i] = MakeApiMask(row, i);
    }
  }

  ASSERT_TRUE(bitmap->MultiplyAlphaMask(mask));

  EXPECT_EQ(FXDIB_Format::kBgra, bitmap->GetFormat());
  for (int row = 0; row < kApiHeight; ++row) {
    auto out_row = bitmap->GetScanlineAs<FX_BGRA_STRUCT<uint8_t>>(row);
    for (int i = 0; i < kApiWidth; ++i) {
      FX_BGRA_STRUCT<uint8_t> expected = MakeApiPixel(row, i);
      expected.alpha =
          static_cast<uint8_t>(expected.alpha * MakeApiMask(row, i) / 255);
      ExpectBgraEq(expected, out_row[i], kApiWidth, i);
    }
  }
}
