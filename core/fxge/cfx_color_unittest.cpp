// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_color.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(CFX_ColorTest, OperatorMinusUnusedComponents) {
  // Construct a Gray color. Unused components (2, 3, 4) should be 0.0f.
  CFX_Color c1 = CFX_Color::MakeGray(0.2f);
  EXPECT_EQ(c1.nColorType, CFX_Color::Type::kGray);
  EXPECT_FLOAT_EQ(c1.fColor1, 0.2f);
  EXPECT_FLOAT_EQ(c1.fColor2, 0.0f);
  EXPECT_FLOAT_EQ(c1.fColor3, 0.0f);
  EXPECT_FLOAT_EQ(c1.fColor4, 0.0f);

  // Subtract a negative value (effectively adding).
  // This should only affect the active component (fColor1).
  CFX_Color c2 = c1 - (-0.5f);

  EXPECT_EQ(c2.nColorType, CFX_Color::Type::kGray);
  EXPECT_FLOAT_EQ(c2.fColor1, 0.7f);

  // These failed before the fix because they became 0.5f.
  EXPECT_FLOAT_EQ(c2.fColor2, 0.0f);
  EXPECT_FLOAT_EQ(c2.fColor3, 0.0f);
  EXPECT_FLOAT_EQ(c2.fColor4, 0.0f);

  // Construct a Gray color with the expected value directly.
  CFX_Color c3 = CFX_Color::MakeGray(0.7f);

  // Should be equal.
  EXPECT_EQ(c2, c3);

  // Construct a Gray color with non-zero unused components.
  CFX_Color c4 = CFX_Color::MakeGray(0.7f);
  c4.fColor2 = 0.5f;
  c4.fColor3 = 0.5f;
  c4.fColor4 = 0.5f;

  // Equality should ignore fColor2, fColor3, and fColor4.
  EXPECT_EQ(c3, c4);
}

TEST(CFX_ColorTest, OperatorSlashUnusedComponents) {
  // Construct a Gray color. Unused components (2, 3, 4) should be 0.0f.
  CFX_Color c1 = CFX_Color::MakeGray(0.8f);

  // Divide.
  CFX_Color c2 = c1 / 2.0f;

  EXPECT_EQ(c2.nColorType, CFX_Color::Type::kGray);
  EXPECT_FLOAT_EQ(c2.fColor1, 0.4f);
  EXPECT_FLOAT_EQ(c2.fColor2, 0.0f);
  EXPECT_FLOAT_EQ(c2.fColor3, 0.0f);
  EXPECT_FLOAT_EQ(c2.fColor4, 0.0f);

  CFX_Color c3 = CFX_Color::MakeGray(0.4f);
  EXPECT_EQ(c2, c3);
}

TEST(CFX_ColorTest, ConvertColorType) {
  // Transparent
  CFX_Color trans;
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            trans.ConvertColorType(CFX_Color::Type::kTransparent).nColorType);
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            trans.ConvertColorType(CFX_Color::Type::kGray).nColorType);
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            trans.ConvertColorType(CFX_Color::Type::kRGB).nColorType);
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            trans.ConvertColorType(CFX_Color::Type::kCMYK).nColorType);

  // Gray
  CFX_Color gray = CFX_Color::MakeGray(0.6f);
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            gray.ConvertColorType(CFX_Color::Type::kTransparent).nColorType);
  EXPECT_EQ(gray, gray.ConvertColorType(CFX_Color::Type::kGray));

  CFX_Color gray_to_rgb = gray.ConvertColorType(CFX_Color::Type::kRGB);
  EXPECT_EQ(CFX_Color::Type::kRGB, gray_to_rgb.nColorType);
  EXPECT_FLOAT_EQ(0.6f, gray_to_rgb.fColor1);
  EXPECT_FLOAT_EQ(0.6f, gray_to_rgb.fColor2);
  EXPECT_FLOAT_EQ(0.6f, gray_to_rgb.fColor3);

  CFX_Color gray_to_cmyk = gray.ConvertColorType(CFX_Color::Type::kCMYK);
  EXPECT_EQ(CFX_Color::Type::kCMYK, gray_to_cmyk.nColorType);
  EXPECT_FLOAT_EQ(0.0f, gray_to_cmyk.fColor1);
  EXPECT_FLOAT_EQ(0.0f, gray_to_cmyk.fColor2);
  EXPECT_FLOAT_EQ(0.0f, gray_to_cmyk.fColor3);
  EXPECT_FLOAT_EQ(0.4f, gray_to_cmyk.fColor4);

  // RGB
  CFX_Color rgb = CFX_Color::MakeRGB(0.2f, 0.4f, 0.6f);
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            rgb.ConvertColorType(CFX_Color::Type::kTransparent).nColorType);
  EXPECT_EQ(rgb, rgb.ConvertColorType(CFX_Color::Type::kRGB));

  CFX_Color rgb_to_gray = rgb.ConvertColorType(CFX_Color::Type::kGray);
  EXPECT_EQ(CFX_Color::Type::kGray, rgb_to_gray.nColorType);
  EXPECT_FLOAT_EQ(0.3f * 0.2f + 0.59f * 0.4f + 0.11f * 0.6f,
                  rgb_to_gray.fColor1);

  CFX_Color rgb_to_cmyk = rgb.ConvertColorType(CFX_Color::Type::kCMYK);
  EXPECT_EQ(CFX_Color::Type::kCMYK, rgb_to_cmyk.nColorType);
  EXPECT_FLOAT_EQ(0.8f, rgb_to_cmyk.fColor1);
  EXPECT_FLOAT_EQ(0.6f, rgb_to_cmyk.fColor2);
  EXPECT_FLOAT_EQ(0.4f, rgb_to_cmyk.fColor3);
  EXPECT_FLOAT_EQ(0.4f, rgb_to_cmyk.fColor4);

  // CMYK
  CFX_Color cmyk = CFX_Color::MakeCMYK(0.1f, 0.2f, 0.3f, 0.4f);
  EXPECT_EQ(CFX_Color::Type::kTransparent,
            cmyk.ConvertColorType(CFX_Color::Type::kTransparent).nColorType);
  EXPECT_EQ(cmyk, cmyk.ConvertColorType(CFX_Color::Type::kCMYK));

  CFX_Color cmyk_to_gray = cmyk.ConvertColorType(CFX_Color::Type::kGray);
  EXPECT_EQ(CFX_Color::Type::kGray, cmyk_to_gray.nColorType);
  EXPECT_FLOAT_EQ(1.0f - (0.3f * 0.1f + 0.59f * 0.2f + 0.11f * 0.3f + 0.4f),
                  cmyk_to_gray.fColor1);

  CFX_Color cmyk_to_rgb = cmyk.ConvertColorType(CFX_Color::Type::kRGB);
  EXPECT_EQ(CFX_Color::Type::kRGB, cmyk_to_rgb.nColorType);
  EXPECT_FLOAT_EQ(0.5f, cmyk_to_rgb.fColor1);
  EXPECT_FLOAT_EQ(0.4f, cmyk_to_rgb.fColor2);
  EXPECT_FLOAT_EQ(0.3f, cmyk_to_rgb.fColor3);

  // Out-of-range inputs exercise fallback paths.
  CFX_Color invalid_gray = CFX_Color::MakeGray(-0.5f);
  EXPECT_EQ(CFX_Color::MakeRGB(0.0f, 0.0f, 0.0f),
            invalid_gray.ConvertColorType(CFX_Color::Type::kRGB));
  EXPECT_EQ(CFX_Color::MakeCMYK(0.0f, 0.0f, 0.0f, 0.0f),
            invalid_gray.ConvertColorType(CFX_Color::Type::kCMYK));

  CFX_Color invalid_rgb = CFX_Color::MakeRGB(2.0f, 0.0f, 0.0f);
  EXPECT_EQ(CFX_Color::MakeGray(0.0f),
            invalid_rgb.ConvertColorType(CFX_Color::Type::kGray));
  EXPECT_EQ(CFX_Color::MakeCMYK(0.0f, 0.0f, 0.0f, 0.0f),
            invalid_rgb.ConvertColorType(CFX_Color::Type::kCMYK));

  CFX_Color invalid_cmyk = CFX_Color::MakeCMYK(0.0f, -1.0f, 0.0f, 0.0f);
  EXPECT_EQ(CFX_Color::MakeGray(0.0f),
            invalid_cmyk.ConvertColorType(CFX_Color::Type::kGray));
  EXPECT_EQ(CFX_Color::MakeRGB(0.0f, 0.0f, 0.0f),
            invalid_cmyk.ConvertColorType(CFX_Color::Type::kRGB));
}
