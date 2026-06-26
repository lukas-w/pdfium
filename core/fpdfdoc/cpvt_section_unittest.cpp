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
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/retain_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

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
