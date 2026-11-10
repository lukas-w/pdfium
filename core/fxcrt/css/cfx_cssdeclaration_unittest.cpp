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

TEST(CFXCSSDeclarationTest, NamedColorParsing) {
  std::optional<CFX_CSSColor> maybe_color =
      CFX_CSSDeclaration::ParseCSSColor(L"red");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFFFF0000u, maybe_color.value());

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"green");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF008000u, maybe_color.value());

  maybe_color = CFX_CSSDeclaration::ParseCSSColor(L"blue");
  ASSERT_TRUE(maybe_color.has_value());
  EXPECT_EQ(0xFF0000FFu, maybe_color.value());

  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSColor(L"notacolor"));
}

TEST(CFXCSSDeclarationTest, ParseCSSString) {
  EXPECT_EQ(L"Double Quoted",
            CFX_CSSDeclaration::ParseCSSString(L"\"Double Quoted\""));
  EXPECT_EQ(L"Single Quoted",
            CFX_CSSDeclaration::ParseCSSString(L"'Single Quoted'"));
  EXPECT_EQ(L"Unquoted", CFX_CSSDeclaration::ParseCSSString(L"Unquoted"));
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSString(L"\"\"").has_value());
  EXPECT_FALSE(CFX_CSSDeclaration::ParseCSSString(L"").has_value());
}

TEST(CFXCSSDeclarationTest, AddProperty) {
  CFX_CSSDeclaration decl;
  EXPECT_TRUE(decl.empty());

  bool important = false;
  decl.AddProperty(CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::Color),
                   L"red");
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::Color, &important));
  EXPECT_FALSE(important);

  important = false;
  decl.AddProperty(CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::FontSize),
                   L"16pt !important");
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::FontSize, &important));
  EXPECT_TRUE(important);

  important = false;
  decl.AddProperty(CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::FontStyle),
                   L"italic");
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::FontStyle, &important));
  EXPECT_FALSE(important);

  important = false;
  decl.AddProperty(CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::FontWeight),
                   L"bold");
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::FontWeight, &important));
  EXPECT_FALSE(important);

  important = false;
  decl.AddProperty(CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::TextAlign),
                   L"center");
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::TextAlign, &important));
  EXPECT_FALSE(important);

  decl.AddProperty(CFX_CSSData::GetPropertyByEnum(CFX_CSSProperty::Border),
                   L"1px");
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::BorderLeftWidth, &important));
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::BorderTopWidth, &important));
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::BorderRightWidth, &important));
  EXPECT_TRUE(decl.GetProperty(CFX_CSSProperty::BorderBottomWidth, &important));

  decl.AddProperty(L"--custom", L"custom_value");
  EXPECT_FALSE(decl.empty());
  EXPECT_EQ(1, std::distance(decl.custom_begin(), decl.custom_end()));
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
