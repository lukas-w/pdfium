// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpvt_variabletext.h"

#include <memory>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fpdfdoc/cpvt_fontmap.h"
#include "core/fpdfdoc/cpvt_word.h"
#include "core/fxcrt/retain_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

// A stub that provides fixed font metrics (like a character width of 10)
// for testing text layout.
class StubProvider : public CPVT_VariableText::Provider {
 public:
  explicit StubProvider(IPVT_FontMap* font_map)
      : CPVT_VariableText::Provider(font_map) {}
  ~StubProvider() override = default;

  // CPVT_VariableText::Provider:
  int GetCharWidth(int32_t nFontIndex, uint16_t word) override { return 10; }
  int32_t GetTypeAscent(int32_t nFontIndex) override { return 10; }
  int32_t GetTypeDescent(int32_t nFontIndex) override { return -2; }
  int32_t GetWordFontIndex(uint16_t word,
                           FX_Charset charset,
                           int32_t nFontIndex) override {
    return 0;
  }
  int32_t GetDefaultFontIndex() override { return 0; }
};

class CPVT_VariableTextTest : public testing::Test {
 public:
  void SetUp() override {
    pdfium::InitializePageModule();
    test_doc_ = std::make_unique<CPDF_TestDocument>();
    font_ = CPDF_Font::GetStockFont(test_doc_.get(), "Helvetica");
    font_map_ = std::make_unique<CPVT_FontMap>(test_doc_.get(), nullptr, font_,
                                               "Helvetica");
    provider_ = std::make_unique<StubProvider>(font_map_.get());
  }

  void TearDown() override {
    provider_.reset();
    font_map_.reset();
    font_.Reset();
    test_doc_.reset();
    pdfium::DestroyPageModule();
  }

 protected:
  std::unique_ptr<CPDF_TestDocument> test_doc_;
  RetainPtr<CPDF_Font> font_;
  std::unique_ptr<CPVT_FontMap> font_map_;
  std::unique_ptr<StubProvider> provider_;
};

TEST_F(CPVT_VariableTextTest, LTRTextLayout) {
  CPVT_VariableText vt(provider_.get());
  vt.SetPlateRect(CFX_FloatRect(0, 0, 100, 100));
  vt.SetFontSize(10.0f);
  vt.SetMultiLine(false);
  vt.SetAutoReturn(false);
  vt.Initialize();

  // "hello" in English (Left-to-Right layout)
  vt.SetText(L"hello");
  vt.RearrangeAll();

  CPVT_VariableText::Iterator* it = vt.GetIterator();
  it->SetAt(1);  // Set to first word index (Place(0, 0, 0))
  CPVT_Word word;
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('h', word.Word);
  float first_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('e', word.Word);
  float second_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('l', word.Word);
  float third_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('l', word.Word);
  float fourth_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ('o', word.Word);
  float fifth_x = word.ptWord.x;

  EXPECT_FALSE(it->NextWord());

  // In LTR, coordinates go left-to-right (0, 10, 20, 30, 40).
  EXPECT_LT(first_x, second_x);
  EXPECT_LT(second_x, third_x);
  EXPECT_LT(third_x, fourth_x);
  EXPECT_LT(fourth_x, fifth_x);
}

TEST_F(CPVT_VariableTextTest, RTLTextLayout) {
  CPVT_VariableText vt(provider_.get());
  vt.SetPlateRect(CFX_FloatRect(0, 0, 100, 100));
  vt.SetFontSize(10.0f);
  vt.SetMultiLine(false);
  vt.SetAutoReturn(false);
  vt.Initialize();

  // Shalom in Hebrew (U+05E9, U+05DC, U+05D5, U+05DD)
  // Logically ordered but should be displayed from right to left.
  vt.SetText(L"\x05E9\x05DC\x05D5\x05DD");
  vt.RearrangeAll();

  CPVT_VariableText::Iterator* it = vt.GetIterator();
  it->SetAt(1);  // Set to first word index (Place(0, 0, 0))
  CPVT_Word word;
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05E9, word.Word);
  float first_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05DC, word.Word);
  float second_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05D5, word.Word);
  float third_x = word.ptWord.x;

  ASSERT_TRUE(it->NextWord());
  ASSERT_TRUE(it->GetWord(word));
  EXPECT_EQ(0x05DD, word.Word);
  float fourth_x = word.ptWord.x;

  EXPECT_FALSE(it->NextWord());

  // TODO(crbug.com/40115028): This should be in RTL order. Currently, it falls
  // back to LTR, so coordinates increase from left to right. Once RTL is
  // supported, the first logical character should have the largest X
  // coordinate, and this test case should use EXPECT_GT instead.
  EXPECT_LT(first_x, second_x);
  EXPECT_LT(second_x, third_x);
  EXPECT_LT(third_x, fourth_x);
}
