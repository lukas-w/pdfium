// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/fax/faxmodule.h"

#include <stdint.h>

#include <array>
#include <memory>

#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/span.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcodec {

namespace {

using ::testing::ElementsAreArray;

// Real-world 44x46 1-D CCITT Group 3 encoded fax stream (containing 38 rows).
constexpr uint8_t kFax1DData[] = {
    0x26, 0xa0, 0x86, 0x80, 0xde, 0x08, 0x1e, 0x11, 0xe1, 0x87, 0x84,
    0x08, 0x30, 0xf4, 0x13, 0x7a, 0x4d, 0xf0, 0xef, 0x4d, 0xbe, 0xdb,
    0xff, 0xff, 0x6f, 0xfd, 0xaf, 0x0c, 0x2f, 0x1f, 0xff, 0x21, 0xb3,
    0x4c, 0x82, 0xe3, 0x30, 0x44, 0xe7, 0xc1, 0x1a, 0x09, 0xc2, 0x05,
    0xe1, 0x05, 0xe9, 0x7a, 0x5e, 0x97, 0xa5, 0xff, 0xa5, 0xfe, 0x0a,
    0x3f, 0x5f, 0xfa, 0xff, 0xfb, 0x5c, 0x9a, 0xaa, 0x30, 0xbe, 0xd2,
    0x7a, 0xdb, 0x4a, 0xd7, 0x0d, 0xa4, 0xda, 0x5b, 0x61, 0x28, 0xf0,
    0xc4, 0x26, 0xb2, 0x16, 0x81, 0x85, 0x00, 0x10, 0x01, 0x00, 0x0a,
};

// Real-world 548x238 2-D CCITT Group 4 encoded fax stream (containing
// 177 rows).
constexpr uint8_t kFaxG4Data[] = {
    0x2e, 0x5e, 0x46, 0xc0, 0xd2, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xc2, 0x20, 0x6c, 0x5e, 0x45, 0xc4, 0x5e, 0x3f,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xfc, 0x5e, 0x40, 0x40, 0x5e, 0x44,
};

}  // namespace

TEST(FaxModuleTest, BadDimensions) {
  // Reject non-positive dimensions.
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 0, 10, 0, false, false, false, 0, 0));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, 0, 0, false, false, false, 0, 0));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, -1, 10, 0, false, false, false, 0, 0));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, -1, 0, false, false, false, 0, 0));

  // Reject dimensions exceeding `kFaxMaxImageDimension`.
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 65536, 10, 0, false, false, false, 0, 0));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, 65536, 0, false, false, false, 0, 0));

  // Reject invalid Columns and Rows overrides.
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, 10, 0, false, false, false, -1, 10));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, 10, 0, false, false, false, 65536, 10));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, 10, 0, false, false, false, 10, -1));
  EXPECT_FALSE(
      FaxModule::CreateDecoder({}, 10, 10, 0, false, false, false, 10, 65536));
}

TEST(FaxModuleTest, EmptyInput) {
  auto decoder =
      FaxModule::CreateDecoder({}, 10, 10, 0, false, false, false, 0, 0);
  ASSERT_TRUE(decoder);
  EXPECT_EQ(10, decoder->GetWidth());
  EXPECT_EQ(10, decoder->GetHeight());
  EXPECT_TRUE(decoder->GetScanline(0).empty());
  EXPECT_EQ(0u, decoder->GetSrcOffset());
}

TEST(FaxModuleTest, Group3_1D_Decode) {
  auto decoder = FaxModule::CreateDecoder(kFax1DData, 44, 46, /*K=*/0,
                                          /*EndOfLine=*/false,
                                          /*EncodedByteAlign=*/false,
                                          /*BlackIs1=*/false,
                                          /*Columns=*/44, /*Rows=*/46);
  ASSERT_TRUE(decoder);
  EXPECT_EQ(44, decoder->GetWidth());
  EXPECT_EQ(46, decoder->GetHeight());

  // Data has 38 rows.
  for (int i = 0; i < 38; ++i) {
    pdfium::span<const uint8_t> line = decoder->GetScanline(i);
    EXPECT_FALSE(line.empty()) << "Failed at line " << i;
    EXPECT_GE(line.size(), 6u);
  }

  // Subsequent rows past the end of stream data should be empty.
  EXPECT_TRUE(decoder->GetScanline(38).empty());

  EXPECT_GT(decoder->GetSrcOffset(), 0u);

  // Rewinding to line 0 should succeed and produce line 0 again.
  pdfium::span<const uint8_t> line0 = decoder->GetScanline(0);
  EXPECT_FALSE(line0.empty());
}

TEST(FaxModuleTest, Group3_1D_BlackIs1) {
  auto normal_decoder = FaxModule::CreateDecoder(
      kFax1DData, 44, 46, /*K=*/0, /*EndOfLine=*/false,
      /*EncodedByteAlign=*/false, /*BlackIs1=*/false, /*Columns=*/44,
      /*Rows=*/46);
  auto black1_decoder = FaxModule::CreateDecoder(
      kFax1DData, 44, 46, /*K=*/0, /*EndOfLine=*/false,
      /*EncodedByteAlign=*/false, /*BlackIs1=*/true, /*Columns=*/44,
      /*Rows=*/46);
  ASSERT_TRUE(normal_decoder);
  ASSERT_TRUE(black1_decoder);

  for (int i = 0; i < 38; ++i) {
    pdfium::span<const uint8_t> normal_line = normal_decoder->GetScanline(i);
    pdfium::span<const uint8_t> black1_line = black1_decoder->GetScanline(i);
    ASSERT_FALSE(normal_line.empty()) << "Line " << i;
    ASSERT_EQ(normal_line.size(), black1_line.size()) << "Line " << i;
    for (size_t j = 0; j < normal_line.size(); ++j) {
      EXPECT_EQ(normal_line[j], static_cast<uint8_t>(~black1_line[j]))
          << "Mismatch at line " << i << " byte " << j;
    }
  }
}

TEST(FaxModuleTest, Group4_Decode) {
  auto decoder = FaxModule::CreateDecoder(kFaxG4Data, 548, 238, /*K=*/-1,
                                          /*EndOfLine=*/false,
                                          /*EncodedByteAlign=*/false,
                                          /*BlackIs1=*/false,
                                          /*Columns=*/548, /*Rows=*/238);
  ASSERT_TRUE(decoder);
  EXPECT_EQ(548, decoder->GetWidth());
  EXPECT_EQ(238, decoder->GetHeight());

  // Data has 177 rows.
  for (int i = 0; i < 177; ++i) {
    pdfium::span<const uint8_t> line = decoder->GetScanline(i);
    EXPECT_FALSE(line.empty()) << "Failed at line " << i;
    EXPECT_GE(line.size(), 69u);
  }

  // Subsequent rows past the end of stream data should be empty.
  EXPECT_TRUE(decoder->GetScanline(177).empty());

  EXPECT_GT(decoder->GetSrcOffset(), 0u);
}

TEST(FaxModuleTest, Group4_FaxG4Decode) {
  constexpr int kWidth = 548;
  constexpr int kHeight = 238;
  constexpr int kPitch = (kWidth + 7) / 8;
  DataVector<uint8_t> dest_buf(kPitch * kHeight);

  uint32_t end_bitpos = FaxModule::FaxG4Decode(
      kFaxG4Data, /*starting_bitpos=*/0, kWidth, kHeight, kPitch, dest_buf);
  EXPECT_GT(end_bitpos, 0u);
  EXPECT_LE(end_bitpos, sizeof(kFaxG4Data) * 8);
}

TEST(FaxModuleTest, EncodedByteAlignRewind) {
  // 4x2 1-D CCITT Group 3 stream with `EncodedByteAlign` enabled:
  // - Byte 0 (0xb0 = 1011_0000): 4-bit white run of 4 (1011), padded with four
  //   zero bits to align to the next byte boundary.
  // - Byte 1 (0x7f = 0111_11_11): 4-bit white run of 2 (0111) + 2-bit black run
  //   of 2 (11), followed by non-zero trailing bits (11) that clear
  //   `byte_align_`.
  static constexpr uint8_t kInput[] = {0xb0, 0x7f};
  static constexpr uint8_t kExpectedRow0[] = {0xff, 0xff, 0xff, 0xff};
  static constexpr uint8_t kExpectedRow1[] = {0xcf, 0xff, 0xff, 0xff};

  auto decoder = FaxModule::CreateDecoder(
      kInput, /*width=*/4, /*height=*/2, /*K=*/0, /*EndOfLine=*/false,
      /*EncodedByteAlign=*/true, /*BlackIs1=*/false, /*Columns=*/4, /*Rows=*/2);
  ASSERT_TRUE(decoder);

  EXPECT_THAT(decoder->GetScanline(0), ElementsAreArray(kExpectedRow0));
  EXPECT_THAT(decoder->GetScanline(1), ElementsAreArray(kExpectedRow1));

  // Reading scanline 0 again rewinds the decoder.
  EXPECT_THAT(decoder->GetScanline(0), ElementsAreArray(kExpectedRow0));
  EXPECT_THAT(decoder->GetScanline(1), ElementsAreArray(kExpectedRow1));
}

}  // namespace fxcodec
