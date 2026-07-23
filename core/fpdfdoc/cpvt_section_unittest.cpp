// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpvt_section.h"

#include <memory>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fpdfdoc/cpvt_fontmap.h"
#include "core/fpdfdoc/cpvt_stub_provider.h"
#include "core/fpdfdoc/cpvt_variabletext.h"
#include "core/fxcrt/cfx_bidi_resolver.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/retain_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
constexpr uint16_t kHebrewAlef = 0x05D0;
constexpr uint16_t kHebrewBet = 0x05D1;
constexpr uint16_t kHebrewGimel = 0x05D2;
}  // namespace

class CPVT_SectionTest : public testing::Test {
 public:
  void SetUp() override {
    pdfium::InitializePageModule();
    test_doc_ = std::make_unique<CPDF_TestDocument>();
    font_ = CPDF_Font::GetStockFont(test_doc_.get(), "Helvetica");
    font_map_ = std::make_unique<CPVT_FontMap>(test_doc_.get(), nullptr, font_,
                                               "Helvetica");
    provider_ = std::make_unique<CPVT_StubProvider>(font_map_.get());
  }

  void TearDown() override {
    provider_.reset();
    font_map_.reset();
    font_.Reset();
    test_doc_.reset();
    pdfium::DestroyPageModule();
  }

 protected:
  void PopulateSectionWithText(CPVT_Section& section,
                               const std::vector<uint32_t>& text) {
    section.SetPlace(CPVT_WordPlace(0, 0, -1));
    for (size_t i = 0; i < text.size(); ++i) {
      section.AddWord(CPVT_WordPlace(0, 0, i),
                      CPVT_WordInfo(text[i], FX_Charset::kDefault, 0));
    }
  }

  void PopulateSectionWithEnglishAndHebrew(CPVT_Section& section) {
    // "A B C " (3 English words + spaces) + "H1 H2" (Alef + space + Bet)
    PopulateSectionWithText(
        section, {'A', ' ', 'B', ' ', 'C', ' ', kHebrewAlef, ' ', kHebrewBet});
  }

  void PopulateSectionWithHebrewAndEnglish(CPVT_Section& section) {
    // "H1 H2 H3 " (3 Hebrew words + spaces) + "A B" (2 English words)
    PopulateSectionWithText(section, {kHebrewAlef, ' ', kHebrewBet, ' ',
                                      kHebrewGimel, ' ', 'A', ' ', 'B'});
  }

  void AssertEnglishAndHebrewLayout(const CPVT_Section& section) {
    ASSERT_GT(section.GetWordArraySize(), 0);

    // Testing PopulateSectionWithEnglishAndHebrew() behavior.
    //
    // TODO(crbug.com/534486929): This asserts the legacy buggy behavior to
    // explicitly document the failure mode. The correct behavior should
    // position the English characters left-to-right, and the Hebrew characters
    // right-to-left within the visual run.
    for (int i = 0; i < section.GetWordArraySize() - 1; ++i) {
      EXPECT_LT(section.GetWordFromArray(i)->fWordX,
                section.GetWordFromArray(i + 1)->fWordX);
    }
  }

  void AssertHebrewAndEnglishLayout(const CPVT_Section& section) {
    ASSERT_GT(section.GetWordArraySize(), 0);

    // Testing PopulateSectionWithHebrewAndEnglish() behavior.
    //
    // TODO(crbug.com/534486929): This asserts the legacy buggy behavior to
    // explicitly document the failure mode. The correct behavior should
    // position the Hebrew characters right-to-left, and the English characters
    // left-to-right within the visual run.
    for (int i = 0; i < section.GetWordArraySize() - 1; ++i) {
      EXPECT_GT(section.GetWordFromArray(i)->fWordX,
                section.GetWordFromArray(i + 1)->fWordX);
    }
  }
  void SetVariableTextDefaults(CPVT_VariableText& vt) {
    vt.SetFontSize(10.0f);
    vt.SetPlateRect(CFX_FloatRect(0, 0, 1000, 1000));
  }

  enum class TextContent { kEnglishAndHebrew, kHebrewAndEnglish };

  void TestTextLayout(TextContent content,
                      CFX_BidiResolver::ParagraphDirection direction) {
    CPVT_VariableText vt(provider_.get());
    SetVariableTextDefaults(vt);
    vt.SetTextDirection(direction);
    vt.Initialize();

    CPVT_Section section(&vt);
    switch (content) {
      case TextContent::kEnglishAndHebrew:
        PopulateSectionWithEnglishAndHebrew(section);
        section.Rearrange();
        AssertEnglishAndHebrewLayout(section);
        return;
      case TextContent::kHebrewAndEnglish:
        PopulateSectionWithHebrewAndEnglish(section);
        section.Rearrange();
        AssertHebrewAndEnglishLayout(section);
        return;
    }
    NOTREACHED();
  }
  void PopulateSectionWithHello(CPVT_Section& section) {
    section.SetPlace(CPVT_WordPlace(0, 0, -1));

    section.AddWord(CPVT_WordPlace(0, 0, 0),
                    CPVT_WordInfo('h', FX_Charset::kDefault, 0));
    section.AddWord(CPVT_WordPlace(0, 0, 1),
                    CPVT_WordInfo('e', FX_Charset::kDefault, 0));
    section.AddWord(CPVT_WordPlace(0, 0, 2),
                    CPVT_WordInfo('l', FX_Charset::kDefault, 0));
    section.AddWord(CPVT_WordPlace(0, 0, 3),
                    CPVT_WordInfo('l', FX_Charset::kDefault, 0));
    section.AddWord(CPVT_WordPlace(0, 0, 4),
                    CPVT_WordInfo('o', FX_Charset::kDefault, 0));

    CPVT_LineInfo lineinfo;
    lineinfo.nBeginWordIndex = 0;
    lineinfo.nEndWordIndex = 4;
    section.AddLine(lineinfo);
    section.ResetLinePlace();
  }

  std::unique_ptr<CPDF_TestDocument> test_doc_;
  RetainPtr<CPDF_Font> font_;
  std::unique_ptr<CPVT_FontMap> font_map_;
  std::unique_ptr<CPVT_StubProvider> provider_;
};

TEST_F(CPVT_SectionTest, ClearLeftWords) {
  CPVT_VariableText vt(provider_.get());
  vt.Initialize();
  CPVT_Section section(&vt);
  PopulateSectionWithHello(section);

  // Clear "hel" [0, 2] by passing in a placeholder for the previous section.
  section.ClearWords(
      CPVT_WordRange(CPVT_WordPlace(-1, 0, -1), CPVT_WordPlace(0, 0, 2)));

  ASSERT_EQ(2, section.GetWordArraySize());
  const CPVT_WordInfo* word0 = section.GetWordFromArray(0);
  ASSERT_TRUE(word0);
  EXPECT_EQ('l', word0->Word);
  const CPVT_WordInfo* word1 = section.GetWordFromArray(1);
  ASSERT_TRUE(word1);
  EXPECT_EQ('o', word1->Word);
}

TEST_F(CPVT_SectionTest, ClearRightWords) {
  CPVT_VariableText vt(provider_.get());
  vt.Initialize();
  CPVT_Section section(&vt);
  PopulateSectionWithHello(section);

  // Clear "llo" [2, 4] by passing in a placeholder for the next section.
  section.ClearWords(
      CPVT_WordRange(CPVT_WordPlace(0, 0, 1), CPVT_WordPlace(1, 0, -1)));

  ASSERT_EQ(2, section.GetWordArraySize());
  const CPVT_WordInfo* word0 = section.GetWordFromArray(0);
  ASSERT_TRUE(word0);
  EXPECT_EQ('h', word0->Word);
  const CPVT_WordInfo* word1 = section.GetWordFromArray(1);
  ASSERT_TRUE(word1);
  EXPECT_EQ('e', word1->Word);
}

TEST_F(CPVT_SectionTest, ClearMidWords) {
  CPVT_VariableText vt(provider_.get());
  vt.Initialize();
  CPVT_Section section(&vt);
  PopulateSectionWithHello(section);

  // Clear "ell" [1, 3] by passing in the word range (0, 3].
  section.ClearWords(
      CPVT_WordRange(CPVT_WordPlace(0, 0, 0), CPVT_WordPlace(0, 0, 3)));

  ASSERT_EQ(2, section.GetWordArraySize());
  const CPVT_WordInfo* word0 = section.GetWordFromArray(0);
  ASSERT_TRUE(word0);
  EXPECT_EQ('h', word0->Word);
  const CPVT_WordInfo* word1 = section.GetWordFromArray(1);
  ASSERT_TRUE(word1);
  EXPECT_EQ('o', word1->Word);
}

TEST_F(CPVT_SectionTest, ClearAllWords) {
  CPVT_VariableText vt(provider_.get());
  vt.Initialize();
  CPVT_Section section(&vt);
  PopulateSectionWithHello(section);

  // Clear all words by passing in placeholders for the previous and next
  // sections.
  section.ClearWords(
      CPVT_WordRange(CPVT_WordPlace(-1, 0, -1), CPVT_WordPlace(1, 0, -1)));

  EXPECT_EQ(0, section.GetWordArraySize());
}

TEST_F(CPVT_SectionTest, OutputLines_EnglishAndHebrew_AutoDirection) {
  TestTextLayout(TextContent::kEnglishAndHebrew,
                 CFX_BidiResolver::ParagraphDirection::kAuto);
}

TEST_F(CPVT_SectionTest, OutputLines_EnglishAndHebrew_ExplicitLeftToRight) {
  TestTextLayout(TextContent::kEnglishAndHebrew,
                 CFX_BidiResolver::ParagraphDirection::kLeftToRight);
}

TEST_F(CPVT_SectionTest, OutputLines_EnglishAndHebrew_ExplicitRightToLeft) {
  TestTextLayout(TextContent::kEnglishAndHebrew,
                 CFX_BidiResolver::ParagraphDirection::kRightToLeft);
}

TEST_F(CPVT_SectionTest, OutputLines_HebrewAndEnglish_AutoDirection) {
  TestTextLayout(TextContent::kHebrewAndEnglish,
                 CFX_BidiResolver::ParagraphDirection::kAuto);
}

TEST_F(CPVT_SectionTest, OutputLines_HebrewAndEnglish_ExplicitLeftToRight) {
  TestTextLayout(TextContent::kHebrewAndEnglish,
                 CFX_BidiResolver::ParagraphDirection::kLeftToRight);
}

TEST_F(CPVT_SectionTest, OutputLines_HebrewAndEnglish_ExplicitRightToLeft) {
  TestTextLayout(TextContent::kHebrewAndEnglish,
                 CFX_BidiResolver::ParagraphDirection::kRightToLeft);
}

TEST_F(CPVT_SectionTest, OutputLines_Multiline_EnglishAndHebrew) {
  CPVT_VariableText vt(provider_.get());
  vt.SetFontSize(10.0f);
  // Narrow width to force word wrap (stub font chars are 0.1 wide)
  vt.SetPlateRect(CFX_FloatRect(0, 0, 0.45f, 1000));
  vt.SetAutoReturn(true);
  vt.Initialize();

  CPVT_Section section(&vt);
  PopulateSectionWithEnglishAndHebrew(section);
  section.Rearrange();

  // With SetAutoReturn(true) and a constrained plate width, the text wraps
  // into 3 lines:
  // Line 1: "A B "
  // Line 2: "C H1 "
  // Line 3: "H2"
  // In the legacy implementation, CFX_BidiString evaluates the overall
  // direction per physical line rather than per paragraph.
  // For Line 2 ("C H1 "), the line-level direction incorrectly resolves to RTL.
  // As a result, the X coordinates for Line 2 progress backwards (X decreases).

  // Line 1: LTR
  for (int i = 0; i < 3; ++i) {
    EXPECT_LT(section.GetWordFromArray(i)->fWordX,
              section.GetWordFromArray(i + 1)->fWordX);
  }

  // TODO(crbug.com/534486929): Line 2: RTL (BUG). This should resolve to LTR
  // once paragraph-level Bidi evaluation is implemented.
  for (int i = 4; i < 7; ++i) {
    EXPECT_GT(section.GetWordFromArray(i)->fWordX,
              section.GetWordFromArray(i + 1)->fWordX);
  }
}
