// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/css/cfx_cssdeclaration.h"

#include <stdint.h>

#include <optional>

#include "core/fxcrt/css/cfx_csscolorvalue.h"
#include "core/fxcrt/css/cfx_csscomputedstyle.h"
#include "core/fxcrt/css/cfx_cssstyleselector.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/widestring.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXCSSDeclarationTest, HexEncodingParsing) {
  std::optional<CFX_CSSColor> maybe_color;

  // Length value invalid.
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSColor(L"#00"));
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSColor(L"#0000"));
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSColor(L"#0000000"));

  // Invalid characters
  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"#zxytlm");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF000000u, maybe_color.value());

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"#000");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF000000u, maybe_color.value());

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"#FFF");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFFFFFFFFu, maybe_color.value());

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"#F0F0F0");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFFF0F0F0u, maybe_color.value());

  // Upper and lower case characters.
  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"#1b2F3c");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF1B2F3Cu, maybe_color.value());
}

TEST(CFXCSSDeclarationTest, RGBEncodingParsing) {
  std::optional<CFX_CSSColor> maybe_color;

  // Invalid input for rgb() syntax.
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSColor(L"blahblahblah"));

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"rgb(0, 0, 0)");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF000000u, maybe_color.value());

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"rgb(128,255,48)");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF80FF30u, maybe_color.value());
}

TEST(CFXCSSDeclarationTest, CustomPropertiesInInlineStyle) {
  CFX_CSSStyleSelector selector;
  RetainPtr<CFX_CSSComputedStyle> style = selector.CreateComputedStyle(nullptr);
  selector.ComputeStyle(
      {}, L"color: #ff0000; xfa-spacerun: yes; tab-interval: 24pt", L"",
      style.Get());

  EXPECT_EQ(0xFFFF0000u, style->GetColor());

  WideString value;
  EXPECT_TRUE(style->GetCustomStyle(L"xfa-spacerun", &value));
  EXPECT_EQ(L"yes", value);

  EXPECT_TRUE(style->GetCustomStyle(L"tab-interval", &value));
  EXPECT_EQ(L"24pt", value);

  EXPECT_FALSE(style->GetCustomStyle(L"nonexistent", &value));
}
