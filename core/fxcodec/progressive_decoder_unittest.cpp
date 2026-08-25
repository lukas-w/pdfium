// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/progressive_decoder.h"

#include <stddef.h>
#include <stdint.h>

#include <array>
#include <numeric>
#include <tuple>
#include <utility>
#include <vector>

#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcrt/cfx_read_only_container_stream.h"
#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcodec {

namespace {

using ::testing::AnyOf;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;

template <size_t Size>
constexpr std::array<uint8_t, Size> IotaArray(uint8_t start) {
  std::array<uint8_t, Size> result;
  std::iota(result.begin(), result.end(), start);
  return result;
}

FXCODEC_STATUS DecodeToBitmap(ProgressiveDecoder& decoder,
                              RetainPtr<CFX_DIBitmap> bitmap) {
  FXCODEC_STATUS status = decoder.StartDecode(std::move(bitmap));
  while (status == FXCODEC_STATUS::kDecodeToBeContinued) {
    status = decoder.ContinueDecode();
  }
  return status;
}

using ProgressiveDecoderTest = testing::Test;

}  // namespace

TEST_F(ProgressiveDecoderTest, Indexed8Bmp) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xc0,
      0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, Indexed8BmpWithInvalidIndex) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xc0,
      0x80, 0x40, 0x00, 0x01, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, Direct24Bmp) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, Direct32Bmp) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0xff};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgrx, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, BmpWithDataOffsetBeforeEndOfHeader) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, BmpWithDataOffsetAfterEndOfHeader) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, TruncatedBmp) {
  // Truncate the input before the pixel data is complete.
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
#if defined(PDF_ENABLE_RUST_BMP)
  // Skia decoder does not detect truncation failure until decoding the bitmap.
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
#else
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
#endif
}

TEST_F(ProgressiveDecoderTest, CorruptBmp) {
  // Invalid BMP header signature.
  static constexpr uint8_t kInput[] = {0x42, 0x4e, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, Os2Bmp) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x1a, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x01, 0x00, 0x01, 0x00, 0x18, 0x00, 0xc0, 0x80, 0x40, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, Indexed1Bmp) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x11,
      0x22, 0x33, 0x00, 0xc0, 0x80, 0x40, 0x00, 0x80, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, Indexed4Bmp) {
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x7a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x10, 0x00,
      0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      // 16 color entries:
      0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      // Pixel data (nibble 1 selects index 1):
      0x10, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, LargeBmp) {
  // Construct a 24-bit BMP larger than `kBlockSize` (4096 bytes).
  static constexpr uint8_t kWidth = 37;
  static constexpr uint8_t kHeight = 38;
  static constexpr size_t kScanlineSize = kWidth * 3 + 1;
  DataVector<uint8_t> input = {
      0x42,    0x4d, 0xd6, 0x10, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00, 0x36,
      0x00,    0x00, 0x00, 0x28, 0x00, 0x00, 0x00, kWidth, 0x00, 0x00, 0x00,
      kHeight, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00,   0x00, 0x00, 0x00,
      0x00,    0xa0, 0x10, 0x00, 0x00, 0x13, 0x0b, 0x00,   0x00, 0x13, 0x0b,
      0x00,    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   0x00, 0x00};
  input.resize(54 + kScanlineSize * kHeight);
  std::iota(input.begin() + 54, input.end(), 0);
  ASSERT_EQ(4310u, input.size());

  ProgressiveDecoder decoder;

  auto source =
      pdfium::MakeRetain<CFX_ReadOnlyDataVectorStream>(std::move(input));
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(kWidth, decoder.GetWidth());
  ASSERT_EQ(kHeight, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);

  for (size_t row = 0; row < kHeight; ++row) {
    // BMP encodes rows from bottom to top by default.
    pdfium::span<const uint8_t> scanline =
        bitmap->GetScanline(kHeight - row - 1);

    EXPECT_THAT(
        scanline.first(kScanlineSize - 1),
        ElementsAreArray(IotaArray<kScanlineSize - 1>(row * kScanlineSize)));

    // Last byte is padding to a 32-bit boundary.
    EXPECT_EQ(0, scanline[kScanlineSize - 1]);
  }
}

TEST_F(ProgressiveDecoderTest, TopDown24Bmp) {
  // Top-down BMP has negative height.
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff,
      0xff, 0xff, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, IntMinHeightBmp) {
  // Height = INT32_MIN (-2147483648).
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x80, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, Direct16BitfieldsBmp) {
  // 16-bit BMP with 5-6-5 bitfields (compression = 3).
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // Red mask: 0x0000F800
      0x00, 0xf8, 0x00, 0x00,
      // Green mask: 0x000007E0
      0xe0, 0x07, 0x00, 0x00,
      // Blue mask: 0x0000001F
      0x1f, 0x00, 0x00, 0x00,
      // Pixel: 0xF81F (Magenta: R=0x1F, G=0x00, B=0x1F)
      0x1f, 0xf8, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
#if defined(PDF_ENABLE_RUST_BMP)
  // Rust/Skia decoders expand 5-bit channels to full 8-bit dynamic range
  // (0xFF).
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xff, 0x00, 0xff, 0x00));
#else
  // C++ decoder shifts 5-bit channels by 3 without replication (0xF8).
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xf8, 0x00, 0xf8, 0x00));
#endif
}

TEST_F(ProgressiveDecoderTest, Direct16OverlappingMasksBitfieldsBmp) {
  // 16-bit BMP with overlapping bitfields masks.
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x42, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // Red mask: 0x0000F800
      0x00, 0xf8, 0x00, 0x00,
      // Overlapping Green mask: 0x0000F800
      0x00, 0xf8, 0x00, 0x00,
      // Blue mask: 0x0000001F
      0x1f, 0x00, 0x00, 0x00,
      // Pixel:
      0x1f, 0xf8, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
#if defined(PDF_ENABLE_RUST_BMP)
  // Skia decoder allows header loading for overlapping masks.
  EXPECT_EQ(FXCODEC_STATUS::kFrameReady, status);
#else
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
#endif
}

TEST_F(ProgressiveDecoderTest, Rle8Bmp) {
  // 8-bit RLE compressed BMP (compression = 1).
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00,
      0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x02, 0x00,
      0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
      // Palette (2 colors):
      0x00, 0x00, 0x00, 0x00, 0xc0, 0x80, 0x40, 0x00,
      // RLE8 data (1 pixel of color 1, then end of bitmap):
      0x01, 0x01, 0x00, 0x01};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgr, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0x00));
}

TEST_F(ProgressiveDecoderTest, ExcessiveDimensionsBmp) {
  // Dimensions 65536 x 65536.
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, Excessive2GBUncompressedBmp) {
  // Dimensions 23171 x 23171 with 32bpp exceeds 2 GB uncompressed buffer size.
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x83, 0x5a, 0x00, 0x00,
      0x83, 0x5a, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, Excessive1GBUncompressedBmp) {
  // Dimensions 16385 x 16384 with 32bpp exceeds 1 GB uncompressed buffer size.
  static constexpr uint8_t kInput[] = {
      0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36,
      0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00,
      0x00, 0x40, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x13, 0x0b, 0x00, 0x00, 0x13, 0x0b,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_BMP, &attr, true);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, Gif87a) {
  static constexpr uint8_t kInput[] = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x01,
      0x00, 0x40, 0x80, 0xc0, 0x80, 0x80, 0x80, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3b};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_GIF, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0xff));
}

TEST_F(ProgressiveDecoderTest, Gif89a) {
  static constexpr uint8_t kInput[] = {
      0x47, 0x49, 0x46, 0x38, 0x39, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80,
      0x01, 0x00, 0x40, 0x80, 0xc0, 0x80, 0x80, 0x80, 0x21, 0xf9, 0x04,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44, 0x01, 0x00, 0x3b};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_GIF, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0), ElementsAre(0xc0, 0x80, 0x40, 0xff));
}

TEST_F(ProgressiveDecoderTest, GifInsufficientCodeSize) {
  // This GIF causes `LZWDecompressor::Create()` to fail because the minimum
  // code size is too small for the palette.
  static constexpr uint8_t kInput[] = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x01, 0x00, 0x01, 0x00, 0x82,
      0x01, 0x00, 0x40, 0x80, 0xc0, 0x80, 0x80, 0x80, 0x81, 0x81, 0x81,
      0x82, 0x82, 0x82, 0x83, 0x83, 0x83, 0x84, 0x84, 0x84, 0x85, 0x85,
      0x85, 0x86, 0x86, 0x86, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x01, 0x00, 0x00, 0x02, 0x2,  0x44, 0x01, 0x00, 0x3b};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_GIF, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

TEST_F(ProgressiveDecoderTest, GifDecodeAcrossScanlines) {
  // This GIF contains an LZW code unit split across 2 scanlines. The decoder
  // must continue decoding the second scanline using the residual data.
  static constexpr uint8_t kInput[] = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x04, 0x00, 0x02, 0x00, 0x80, 0x01,
      0x00, 0x40, 0x80, 0xc0, 0x80, 0x80, 0x80, 0x2c, 0x00, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03, 0x84, 0x6f, 0x05, 0x00, 0x3b};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_GIF, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(4, decoder.GetWidth());
  ASSERT_EQ(2, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0),
              ElementsAre(0xc0, 0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff, 0xc0,
                          0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff));
  EXPECT_THAT(bitmap->GetScanline(1),
              ElementsAre(0xc0, 0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff, 0xc0,
                          0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff));
}

TEST_F(ProgressiveDecoderTest, GifDecodeAcrossSubblocks) {
  // This GIF contains a scanline split across 2 data sub-blocks. The decoder
  // must continue decoding in the second sub-block.
  static constexpr uint8_t kInput[] = {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x04, 0x00, 0x02, 0x00,
      0x80, 0x01, 0x00, 0x40, 0x80, 0xc0, 0x80, 0x80, 0x80, 0x2c,
      0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x02,
      0x02, 0x84, 0x6f, 0x01, 0x05, 0x00, 0x3b};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_GIF, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(4, decoder.GetWidth());
  ASSERT_EQ(2, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0),
              ElementsAre(0xc0, 0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff, 0xc0,
                          0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff));
  EXPECT_THAT(bitmap->GetScanline(1),
              ElementsAre(0xc0, 0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff, 0xc0,
                          0x80, 0x40, 0xff, 0xc0, 0x80, 0x40, 0xff));
}

// `kGreenPng` has been taken from Chromium's
// `//third_party/blink/renderer/platform/testing/data/green.png`.
constexpr std::array<const uint8_t, 87> kGreenPng = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00,
    0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00,
    0x00, 0x32, 0x01, 0x03, 0x00, 0x00, 0x00, 0x90, 0xfb, 0xec, 0xfd,
    0x00, 0x00, 0x00, 0x03, 0x50, 0x4c, 0x54, 0x45, 0x00, 0xff, 0x00,
    0x34, 0x5e, 0xc0, 0xa8, 0x00, 0x00, 0x00, 0x0f, 0x49, 0x44, 0x41,
    0x54, 0x28, 0x15, 0x63, 0x60, 0x18, 0x05, 0xa3, 0x60, 0x68, 0x02,
    0x00, 0x02, 0xbc, 0x00, 0x01, 0x1b, 0xdd, 0xe3, 0x90, 0x00, 0x00,
    0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

TEST_F(ProgressiveDecoderTest, PngSmokeTest) {
  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kGreenPng);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_PNG, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(100, decoder.GetWidth());
  ASSERT_EQ(50, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0).first(4u),
              ElementsAre(0x00, 0xFF, 0x00, 0xFF));
}

TEST_F(ProgressiveDecoderTest, TruncatedPng) {
  // Truncate the input in the middle of the IDAT chunk's payload.
  const auto input = pdfium::span(kGreenPng).first(64u);

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(input);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_PNG, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(100, decoder.GetWidth());
  ASSERT_EQ(50, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kError, status);
}

// One motivation for the `BigPng` test is to ensure that tests cover multiple
// iterations of the loop inside `ProgressiveDecoder::PngContinueDecode` or
// `ProgressiveDecoder::PngDetectImageTypeInBuffer` (i.e.  covering the scenario
// where decoding happens over multiple file chunks).
//
// In particular, construcing an `SkCodec` is only possible _after_ parsing
// image metadata (including the color profile, etc.), which postpones PNG
// detection all the way until encountering an `IDAT` chunk.  This means that
// a Skia-based decoder should *not* consume input when constructing an
// `SkCodec` returns `kIncompleteInput` error - instead the next iteration
// of `PngReadMoreData` should retry _from the start_ with a _bigger_ buffer.
TEST_F(ProgressiveDecoderTest, BigPng) {
  // Split `kGreenPng` into a prefix (before `IDAT` chunk) and a suffix (`IDAT`
  // and subsequent chunks).  Inserting 0, 1, or more (mostly inert)
  // `kTextChunk` bytes in the middle allows generating arbitrarily large test
  // inputs.
  const auto kPrefix = pdfium::span(kGreenPng).first(48u);
  const auto kSuffix = pdfium::span(kGreenPng).subspan(48u);
  CHECK_EQ(kSuffix[4], static_cast<uint8_t>('I'));
  CHECK_EQ(kSuffix[5], static_cast<uint8_t>('D'));
  CHECK_EQ(kSuffix[6], static_cast<uint8_t>('A'));
  CHECK_EQ(kSuffix[7], static_cast<uint8_t>('T'));

  // `tEXt` chunk (mostly ignored during decoding process).
  static constexpr std::array<const uint8_t, 32> kTextChunk = {
      0x00, 0x00, 0x00, 0x14, 0x74, 0x45, 0x58, 0x74, 0x54, 0x65, 0x73,
      0x74, 0x00, 0x50, 0x44, 0x46, 0x69, 0x75, 0x6D, 0x20, 0x53, 0x75,
      0x69, 0x74, 0x65, 0x2E, 0x2E, 0x2E, 0x11, 0x22, 0x17, 0x91};

  // `kBlockSize` in `progressive_decoder.cpp` is 4096 - let's therefore
  // construct `input` that is a few multiples of that long.
  std::vector<uint8_t> input;
  input.insert(input.end(), kPrefix.begin(), kPrefix.end());
  while (input.size() < 4096 * 10) {
    input.insert(input.end(), kTextChunk.begin(), kTextChunk.end());
  }
  input.insert(input.end(), kSuffix.begin(), kSuffix.end());

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(input);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_PNG, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(100, decoder.GetWidth());
  ASSERT_EQ(50, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  EXPECT_THAT(bitmap->GetScanline(0).first(4u),
              ElementsAre(0x00, 0xFF, 0x00, 0xFF));
}

// 1x1 8-bit grayscale PNG with a gAMA chunk of 100,000 (gamma 1.0) and pixel
// value 128 (0x80).
TEST_F(ProgressiveDecoderTest, PngGammaChunk) {
  static constexpr uint8_t kInput[] = {
      // Signature (8 bytes)
      0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
      // IHDR chunk (25 bytes): 1x1, 8-bit grayscale
      0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b,
      0x55,
      // gAMA chunk (16 bytes): value 100,000 (1.0 in 100k fixed-point)
      0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x01, 0x86, 0xa0,
      0x31, 0xe8, 0x96, 0x5f,
      // IDAT chunk (22 bytes): filter=0, pixel=0x80 (128)
      0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0x63, 0x68,
      0x00, 0x00, 0x00, 0x82, 0x00, 0x81, 0xda, 0x45, 0x08, 0x3b,
      // IEND chunk (12 bytes)
      0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82};

  ProgressiveDecoder decoder;

  auto source = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kInput);
  CFX_DIBAttribute attr;
  FXCODEC_STATUS status =
      decoder.LoadImageInfo(std::move(source), FXCODEC_IMAGE_PNG, &attr, true);
  ASSERT_EQ(FXCODEC_STATUS::kFrameReady, status);

  ASSERT_EQ(1, decoder.GetWidth());
  ASSERT_EQ(1, decoder.GetHeight());
  ASSERT_EQ(FXDIB_Format::kBgra, decoder.GetBitmapFormat());

  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(decoder.GetWidth(), decoder.GetHeight(),
                             decoder.GetBitmapFormat()));

  size_t frames;
  std::tie(status, frames) = decoder.GetFrames();
  ASSERT_EQ(FXCODEC_STATUS::kDecodeReady, status);
  ASSERT_EQ(1u, frames);

  status = DecodeToBitmap(decoder, bitmap);
  EXPECT_EQ(FXCODEC_STATUS::kDecodeFinished, status);
  // Input 0x80 with file_gamma=1.0 and display_gamma=2.2 yields 0xBA (186
  // floor) or 0xBB (187 rounded).
  EXPECT_THAT(bitmap->GetScanline(0).first(4u),
              AnyOf(ElementsAre(0xBA, 0xBA, 0xBA, 0xFF),
                    ElementsAre(0xBB, 0xBB, 0xBB, 0xFF)));
}

}  // namespace fxcodec
