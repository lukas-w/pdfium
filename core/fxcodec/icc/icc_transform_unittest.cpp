// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/icc/icc_transform.h"

#include <stdint.h>

#include <array>
#include <memory>

#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace fxcodec {

namespace {

DataVector<uint8_t> SerializeProfile(cmsHPROFILE profile) {
  cmsUInt32Number bytes_needed = 0;
  if (!cmsSaveProfileToMem(profile, nullptr, &bytes_needed) ||
      bytes_needed == 0) {
    return {};
  }
  DataVector<uint8_t> buffer(bytes_needed);
  if (!cmsSaveProfileToMem(profile, buffer.data(), &bytes_needed)) {
    return {};
  }
  return buffer;
}

}  // namespace

TEST(IccTransformTest, IsValidIccComponents) {
  EXPECT_TRUE(IccTransform::IsValidIccComponents(1));
  EXPECT_TRUE(IccTransform::IsValidIccComponents(3));
  EXPECT_TRUE(IccTransform::IsValidIccComponents(4));

  EXPECT_FALSE(IccTransform::IsValidIccComponents(-1));
  EXPECT_FALSE(IccTransform::IsValidIccComponents(0));
  EXPECT_FALSE(IccTransform::IsValidIccComponents(2));
  EXPECT_FALSE(IccTransform::IsValidIccComponents(5));
  EXPECT_FALSE(IccTransform::IsValidIccComponents(16));
}

TEST(IccTransformTest, InvalidProfiles) {
  EXPECT_FALSE(IccTransform::CreateTransformSRGB({}));

  static constexpr uint8_t kGarbage[] = {0x00, 0x01, 0x02, 0x03, 0x04};
  EXPECT_FALSE(IccTransform::CreateTransformSRGB(kGarbage));
}

TEST(IccTransformTest, SRGBProfile) {
  cmsHPROFILE profile = cmsCreate_sRGBProfile();
  ASSERT_TRUE(profile);
  DataVector<uint8_t> profile_data = SerializeProfile(profile);
  cmsCloseProfile(profile);
  ASSERT_FALSE(profile_data.empty());

  std::unique_ptr<IccTransform> transform =
      IccTransform::CreateTransformSRGB(profile_data);
  ASSERT_TRUE(transform);

  // Translate single pixel float components.
  static constexpr float kSrcRgb[3] = {1.0f, 0.0f, 0.0f};
  float dst_rgb[3] = {0.0f, 0.0f, 0.0f};
  transform->Translate(kSrcRgb, dst_rgb);
  EXPECT_NEAR(dst_rgb[0], 1.0f, 0.05f);
  EXPECT_NEAR(dst_rgb[1], 0.0f, 0.05f);
  EXPECT_NEAR(dst_rgb[2], 0.0f, 0.05f);

  // Translate scanline with 8-bit samples.
  // Note that destination is BGR_8 format per cmsCreateTransform.
  static constexpr uint8_t kSrcPixel[3] = {255, 0, 0};
  uint8_t dst_pixel[3] = {0, 0, 0};
  transform->TranslateScanline(dst_pixel, kSrcPixel, 1);
  EXPECT_EQ(dst_pixel[0], 0);    // B
  EXPECT_EQ(dst_pixel[1], 0);    // G
  EXPECT_EQ(dst_pixel[2], 255);  // R
}

TEST(IccTransformTest, LabProfile) {
  cmsHPROFILE profile = cmsCreateLab4Profile(nullptr);
  ASSERT_TRUE(profile);
  DataVector<uint8_t> profile_data = SerializeProfile(profile);
  cmsCloseProfile(profile);
  ASSERT_FALSE(profile_data.empty());

  std::unique_ptr<IccTransform> transform =
      IccTransform::CreateTransformSRGB(profile_data);
  ASSERT_TRUE(transform);

  // L*=100, a*=0, b*=0 is diffuse white.
  static constexpr FX_LAB_STRUCT<float> kLabWhite = {100.0f, 0.0f, 0.0f};
  FX_RGB_STRUCT<float> dst_rgb = {};
  transform->Translate(
      fxcrt::reinterpret_span<const float>(pdfium::span_from_ref(kLabWhite)),
      fxcrt::reinterpret_span<float>(pdfium::span_from_ref(dst_rgb))
          .first<3>());
  EXPECT_NEAR(dst_rgb.red, 1.0f, 0.05f);
  EXPECT_NEAR(dst_rgb.green, 1.0f, 0.05f);
  EXPECT_NEAR(dst_rgb.blue, 1.0f, 0.05f);

  // Translate scanline with 8-bit Lab samples.
  // 1 pixel = 3 bytes (L=255, a=128, b=128 is diffuse white).
  // Note that destination is BGR_8 format per cmsCreateTransform.
  static constexpr FX_LAB_STRUCT<uint8_t> kLabWhitePixel = {255, 128, 128};
  FX_BGR_STRUCT<uint8_t> dst_pixel = {};
  transform->TranslateScanline(pdfium::byte_span_from_ref(dst_pixel),
                               pdfium::byte_span_from_ref(kLabWhitePixel), 1);
  EXPECT_EQ(dst_pixel.blue, 255);
  EXPECT_EQ(dst_pixel.green, 255);
  EXPECT_EQ(dst_pixel.red, 255);

  // Translate scanline with multiple 8-bit Lab samples (2 pixels).
  static constexpr std::array<FX_LAB_STRUCT<uint8_t>, 2> kLabTwoPixels = {
      {{255, 128, 128}, {255, 128, 128}}};
  std::array<FX_BGR_STRUCT<uint8_t>, 2> dst_two_pixels = {};
  transform->TranslateScanline(pdfium::byte_span_from_ref(dst_two_pixels),
                               pdfium::byte_span_from_ref(kLabTwoPixels), 2);
  EXPECT_EQ(dst_two_pixels[0].blue, 255);
  EXPECT_EQ(dst_two_pixels[0].green, 255);
  EXPECT_EQ(dst_two_pixels[0].red, 255);
  EXPECT_EQ(dst_two_pixels[1].blue, 255);
  EXPECT_EQ(dst_two_pixels[1].green, 255);
  EXPECT_EQ(dst_two_pixels[1].red, 255);
}

TEST(IccTransformTest, GrayProfile) {
  cmsToneCurve* gamma = cmsBuildGamma(nullptr, 2.2);
  ASSERT_TRUE(gamma);
  cmsHPROFILE profile = cmsCreateGrayProfile(nullptr, gamma);
  cmsFreeToneCurve(gamma);
  ASSERT_TRUE(profile);
  DataVector<uint8_t> profile_data = SerializeProfile(profile);
  cmsCloseProfile(profile);
  ASSERT_FALSE(profile_data.empty());

  std::unique_ptr<IccTransform> transform =
      IccTransform::CreateTransformSRGB(profile_data);
  ASSERT_TRUE(transform);

  static constexpr float kGrayWhite[1] = {1.0f};
  float dst_rgb[3] = {0.0f, 0.0f, 0.0f};
  transform->Translate(kGrayWhite, dst_rgb);
  EXPECT_NEAR(dst_rgb[0], 1.0f, 0.05f);
  EXPECT_NEAR(dst_rgb[1], 1.0f, 0.05f);
  EXPECT_NEAR(dst_rgb[2], 1.0f, 0.05f);
}

}  // namespace fxcodec
