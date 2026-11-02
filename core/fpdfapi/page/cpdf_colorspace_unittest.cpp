// Copyright 2021 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/page/cpdf_colorspace.h"

#include <stddef.h>
#include <stdint.h>

#include <algorithm>

#include "core/fxcrt/retain_ptr.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAre;

TEST(CPDFCalGrayTest, TranslateImageLine) {
  RetainPtr<CPDF_ColorSpace> pCal = CPDF_ColorSpace::AllocateColorSpace("CalG");
  ASSERT_TRUE(pCal);

  const uint8_t kSrc[12] = {255, 0, 0, 0, 255, 0, 0, 0, 255, 128, 128, 128};
  uint8_t dst[12];
  std::ranges::fill(dst, 0xbd);
  // Transparency masks are only used with CMYK colorspaces, not CalGray.
  pCal->TranslateImageLine(dst, kSrc, 4, 4, 1, /*is_transparency_mask=*/false);
  EXPECT_THAT(dst, ElementsAre(255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0));
}

TEST(CPDFCalRGBTest, TranslateImageLine) {
  RetainPtr<CPDF_ColorSpace> pCal = CPDF_ColorSpace::AllocateColorSpace("CalR");
  ASSERT_TRUE(pCal);

  const uint8_t kSrc[12] = {255, 0, 0, 0, 255, 0, 0, 0, 255, 128, 128, 128};
  uint8_t dst[12];
  std::ranges::fill(dst, 0xbd);
  // Transparency masks are only used with CMYK colorspaces, not CalRGB.
  pCal->TranslateImageLine(dst, kSrc, 4, 4, 1, /*is_transparency_mask=*/false);
  EXPECT_THAT(dst, ElementsAre(0, 0, 255, 0, 255, 0, 255, 0, 0, 128, 128, 128));
}
