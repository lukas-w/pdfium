// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/jpegmodule.h"

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/span.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

namespace fxcodec {

namespace {

std::vector<uint8_t> LoadMonaLisa() {
  std::string file_path = PathService::GetTestFilePath("mona_lisa.jpg");
  return GetFileContents(file_path.c_str());
}

}  // namespace

TEST(JpegModuleTest, LoadInfoInvalid) {
  EXPECT_FALSE(JpegModule::LoadInfo({}).has_value());

  std::vector<uint8_t> invalid = {0x00, 0x01, 0x02, 0x03};
  EXPECT_FALSE(JpegModule::LoadInfo(invalid).has_value());
}

TEST(JpegModuleTest, LoadInfoValid) {
  std::vector<uint8_t> jpeg_data = LoadMonaLisa();
  ASSERT_FALSE(jpeg_data.empty());

  std::optional<JpegModule::ImageInfo> info = JpegModule::LoadInfo(jpeg_data);
  ASSERT_TRUE(info.has_value());
  EXPECT_EQ(120u, info->width);
  EXPECT_EQ(120u, info->height);
  EXPECT_EQ(3, info->num_components);
  EXPECT_EQ(8, info->bits_per_components);
}

TEST(JpegModuleTest, CreateDecoderInvalid) {
  EXPECT_FALSE(JpegModule::CreateDecoder({}, 120, 120, 3, true, 1));

  std::vector<uint8_t> invalid = {0x00, 0x01, 0x02, 0x03};
  EXPECT_FALSE(JpegModule::CreateDecoder(invalid, 120, 120, 3, true, 1));
}

TEST(JpegModuleTest, CreateDecoderValid) {
  std::vector<uint8_t> jpeg_data = LoadMonaLisa();
  ASSERT_FALSE(jpeg_data.empty());

  std::unique_ptr<ScanlineDecoder> decoder =
      JpegModule::CreateDecoder(jpeg_data, 120, 120, 3, true, 1);
  ASSERT_TRUE(decoder);
  EXPECT_EQ(120, decoder->GetWidth());
  EXPECT_EQ(120, decoder->GetHeight());
  EXPECT_EQ(3, decoder->CountComps());
  EXPECT_EQ(8, decoder->GetBPC());

  for (int line = 0; line < 120; ++line) {
    pdfium::span<const uint8_t> scanline = decoder->GetScanline(line);
    EXPECT_FALSE(scanline.empty()) << "line " << line;
    EXPECT_GE(scanline.size(), 360u) << "line " << line;
  }

  // Out of range scanline line index.
  EXPECT_TRUE(decoder->GetScanline(120).empty());

  // Rewind and re-read line 0.
  pdfium::span<const uint8_t> scanline0 = decoder->GetScanline(0);
  EXPECT_FALSE(scanline0.empty());
  EXPECT_GE(scanline0.size(), 360u);
}

TEST(JpegModuleTest, CreateDecoderDownscaled) {
  std::vector<uint8_t> jpeg_data = LoadMonaLisa();
  ASSERT_FALSE(jpeg_data.empty());

  // 1/2 scale (scale_denom = 2).
  std::unique_ptr<ScanlineDecoder> decoder2 =
      JpegModule::CreateDecoder(jpeg_data, 120, 120, 3, true, 2);
  ASSERT_TRUE(decoder2);
  EXPECT_EQ(60, decoder2->GetWidth());
  EXPECT_EQ(60, decoder2->GetHeight());

  for (int line = 0; line < 60; ++line) {
    pdfium::span<const uint8_t> scanline = decoder2->GetScanline(line);
    EXPECT_FALSE(scanline.empty()) << "line " << line;
    EXPECT_GE(scanline.size(), 180u) << "line " << line;
  }

  // 1/4 scale (scale_denom = 4).
  std::unique_ptr<ScanlineDecoder> decoder4 =
      JpegModule::CreateDecoder(jpeg_data, 120, 120, 3, true, 4);
  ASSERT_TRUE(decoder4);
  EXPECT_EQ(30, decoder4->GetWidth());
  EXPECT_EQ(30, decoder4->GetHeight());

  for (int line = 0; line < 30; ++line) {
    pdfium::span<const uint8_t> scanline = decoder4->GetScanline(line);
    EXPECT_FALSE(scanline.empty()) << "line " << line;
    EXPECT_GE(scanline.size(), 90u) << "line " << line;
  }

  // 1/8 scale (scale_denom = 8).
  std::unique_ptr<ScanlineDecoder> decoder8 =
      JpegModule::CreateDecoder(jpeg_data, 120, 120, 3, true, 8);
  ASSERT_TRUE(decoder8);
  EXPECT_EQ(15, decoder8->GetWidth());
  EXPECT_EQ(15, decoder8->GetHeight());

  for (int line = 0; line < 15; ++line) {
    pdfium::span<const uint8_t> scanline = decoder8->GetScanline(line);
    EXPECT_FALSE(scanline.empty()) << "line " << line;
    EXPECT_GE(scanline.size(), 45u) << "line " << line;
  }
}

}  // namespace fxcodec
