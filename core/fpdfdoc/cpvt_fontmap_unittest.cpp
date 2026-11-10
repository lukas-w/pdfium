// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpvt_fontmap.h"

#include <memory>

#include "core/fpdfapi/font/cpdf_font.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fxcrt/retain_ptr.h"
#include "testing/gtest/include/gtest/gtest.h"

class CPVT_FontMapTest : public testing::Test {
 public:
  void SetUp() override {
    pdfium::InitializePageModule();
    test_doc_ = std::make_unique<CPDF_TestDocument>();
    test_doc_->SetRoot(pdfium::MakeRetain<CPDF_Dictionary>());
    def_font_ = CPDF_Font::GetStockFont(test_doc_.get(), "Helvetica");
    ASSERT_TRUE(def_font_);
  }

  void TearDown() override {
    def_font_.Reset();
    test_doc_.reset();
    pdfium::DestroyPageModule();
  }

  CPDF_TestDocument* doc() { return test_doc_.get(); }
  RetainPtr<CPDF_Font> def_font() { return def_font_; }

 private:
  std::unique_ptr<CPDF_TestDocument> test_doc_;
  RetainPtr<CPDF_Font> def_font_;
};

TEST_F(CPVT_FontMapTest, DefaultFontAndBounds) {
  CPVT_FontMap font_map(doc(), nullptr, def_font(), "Helvetica");

  EXPECT_EQ(def_font(), font_map.GetPDFFont(0));
  EXPECT_EQ("Helvetica", font_map.GetPDFFontAlias(0));

  EXPECT_FALSE(font_map.GetPDFFont(-1));
  EXPECT_TRUE(font_map.GetPDFFontAlias(-1).IsEmpty());

  EXPECT_FALSE(font_map.GetPDFFont(2));
  EXPECT_TRUE(font_map.GetPDFFontAlias(2).IsEmpty());
}

TEST_F(CPVT_FontMapTest, NullDocumentSysFont) {
  auto res_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  CPVT_FontMap font_map(nullptr, res_dict, def_font(), "Helvetica");

  EXPECT_FALSE(font_map.GetPDFFont(1));
  EXPECT_TRUE(font_map.GetPDFFontAlias(1).IsEmpty());
}

TEST_F(CPVT_FontMapTest, NullResDictSysFont) {
  CPVT_FontMap font_map(doc(), nullptr, def_font(), "Helvetica");

  EXPECT_FALSE(font_map.GetPDFFont(1));
  EXPECT_TRUE(font_map.GetPDFFontAlias(1).IsEmpty());
}

TEST_F(CPVT_FontMapTest, ValidResDictSysFont) {
  auto res_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  auto font_list = res_dict->SetNewFor<CPDF_Dictionary>("Font");
  CPVT_FontMap font_map(doc(), res_dict, def_font(), "Helvetica");

  RetainPtr<CPDF_Font> sys_font = font_map.GetPDFFont(1);
  ByteString sys_alias = font_map.GetPDFFontAlias(1);

  if (sys_font) {
    EXPECT_FALSE(sys_alias.IsEmpty());
    EXPECT_TRUE(font_list->KeyExist(sys_alias.AsStringView()));

    // Repeated calls return the cached system font and alias.
    EXPECT_EQ(sys_font, font_map.GetPDFFont(1));
    EXPECT_EQ(sys_alias, font_map.GetPDFFontAlias(1));
  } else {
    EXPECT_TRUE(sys_alias.IsEmpty());
  }
}
