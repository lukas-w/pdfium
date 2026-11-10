// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/css/cfx_cssstyleselector.h"

#include <memory>
#include <utility>
#include <vector>

#include "core/fxcrt/css/cfx_csscomputedstyle.h"
#include "core/fxcrt/css/cfx_cssdeclaration.h"
#include "core/fxcrt/css/cfx_cssstylerule.h"
#include "core/fxcrt/css/cfx_cssstylesheet.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/widestring.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CFXCSSStyleSelectorTest, DefaultFontSize) {
  CFX_CSSStyleSelector selector;
  selector.SetDefaultFontSize(18.0f);
  RetainPtr<CFX_CSSComputedStyle> style = selector.CreateComputedStyle(nullptr);
  ASSERT_TRUE(style);
  selector.ComputeStyle({}, L"font-size: medium", L"", style.Get());
  EXPECT_FLOAT_EQ(18.0f, style->GetFontSize());
}

TEST(CFXCSSStyleSelectorTest, ComputeStyleInline) {
  CFX_CSSStyleSelector selector;
  RetainPtr<CFX_CSSComputedStyle> style = selector.CreateComputedStyle(nullptr);
  ASSERT_TRUE(style);
  selector.ComputeStyle(
      {}, L"color: red; font-size: 20pt; text-align: right; font-style: italic",
      L"", style.Get());

  EXPECT_EQ(0xFFFF0000u, style->GetColor());
  EXPECT_EQ(CFX_CSSTextAlign::Right, style->GetTextAlign());
  EXPECT_EQ(CFX_CSSFontStyle::Italic, style->GetFontStyle());
}

TEST(CFXCSSStyleSelectorTest, ComputeStyleWithAlignString) {
  CFX_CSSStyleSelector selector;
  RetainPtr<CFX_CSSComputedStyle> style = selector.CreateComputedStyle(nullptr);
  ASSERT_TRUE(style);
  selector.ComputeStyle({}, L"", L"center", style.Get());
  EXPECT_EQ(CFX_CSSTextAlign::Center, style->GetTextAlign());

  selector.ComputeStyle({}, L"", L"justify", style.Get());
  EXPECT_EQ(CFX_CSSTextAlign::Justify, style->GetTextAlign());
}

TEST(CFXCSSStyleSelectorTest, ComputeStyleInheritance) {
  CFX_CSSStyleSelector selector;
  RetainPtr<CFX_CSSComputedStyle> parent_style =
      selector.CreateComputedStyle(nullptr);
  ASSERT_TRUE(parent_style);
  selector.ComputeStyle({}, L"color: #0000ff; font-weight: bold", L"",
                        parent_style.Get());

  RetainPtr<CFX_CSSComputedStyle> child_style =
      selector.CreateComputedStyle(parent_style.Get());
  ASSERT_TRUE(child_style);
  selector.ComputeStyle({}, L"", L"", child_style.Get());

  // Inherited properties should match parent.
  EXPECT_EQ(0xFF0000FFu, child_style->GetColor());
  EXPECT_EQ(700, child_style->GetFontWeight());
}

TEST(CFXCSSStyleSelectorTest, MatchDeclarationsWithUAStyleSheet) {
  CFX_CSSStyleSelector selector;
  auto sheet = std::make_unique<CFX_CSSStyleSheet>();
  EXPECT_TRUE(sheet->LoadBuffer(L"p { color: blue; } h1 { font-size: 24pt; }"));
  ASSERT_EQ(2u, sheet->CountRules());
  ASSERT_TRUE(sheet->GetRule(0));
  ASSERT_TRUE(sheet->GetRule(1));
  const CFX_CSSDeclaration* p_decl = sheet->GetRule(0)->GetDeclaration();
  const CFX_CSSDeclaration* h1_decl = sheet->GetRule(1)->GetDeclaration();

  selector.SetUAStyleSheet(std::move(sheet));
  selector.UpdateStyleIndex();

  EXPECT_THAT(selector.MatchDeclarations(L"p"), testing::ElementsAre(p_decl));
  EXPECT_THAT(selector.MatchDeclarations(L"h1"), testing::ElementsAre(h1_decl));
  EXPECT_THAT(selector.MatchDeclarations(L"unknown_tag"), testing::IsEmpty());
}
