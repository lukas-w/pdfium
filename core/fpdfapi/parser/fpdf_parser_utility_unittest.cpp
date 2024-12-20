// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/fpdf_parser_utility.h"

#include <memory>

#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pdfium {

TEST(ParserUtilityTest, NameDecode) {
  EXPECT_EQ("", PDF_NameDecode(""));
  EXPECT_EQ("A", PDF_NameDecode("A"));
  EXPECT_EQ("#", PDF_NameDecode("#"));
  EXPECT_EQ("#4", PDF_NameDecode("#4"));
  EXPECT_EQ("A", PDF_NameDecode("#41"));
  EXPECT_EQ("A1", PDF_NameDecode("#411"));
}

TEST(ParserUtilityTest, NameEncode) {
  EXPECT_EQ("", PDF_NameEncode(""));
  EXPECT_EQ("A", PDF_NameEncode("A"));
  EXPECT_EQ("#23", PDF_NameEncode("#"));
  EXPECT_EQ("#20", PDF_NameEncode(" "));
  EXPECT_EQ("!@#23$#25^&*#28#29#3C#3E#5B#5D", PDF_NameEncode("!@#$%^&*()<>[]"));
  EXPECT_EQ("#C2", PDF_NameEncode("\xc2"));
  EXPECT_EQ("f#C2#A5", PDF_NameEncode("f\xc2\xa5"));
}

TEST(ParserUtilityTest, ValidateDictType) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();

  // No type.
  EXPECT_FALSE(ValidateDictType(dict.Get(), "foo"));
  EXPECT_FALSE(ValidateDictType(dict.Get(), "bar"));

  // Add the wrong object type.
  dict->SetNewFor<CPDF_String>("Type", L"foo");
  EXPECT_FALSE(ValidateDictType(dict.Get(), "foo"));
  EXPECT_FALSE(ValidateDictType(dict.Get(), "bar"));

  // Add the correct object type.
  dict->SetNewFor<CPDF_Name>("Type", "foo");
  EXPECT_TRUE(ValidateDictType(dict.Get(), "foo"));
  EXPECT_FALSE(ValidateDictType(dict.Get(), "bar"));
}

TEST(ParserUtilityTest, ValidateDictAllResourcesOfType) {
  InitializePageModule();

  {
    // Direct dictionary.
    auto dict = pdfium::MakeRetain<CPDF_Dictionary>();

    // Empty dict is ok.
    EXPECT_TRUE(ValidateDictAllResourcesOfType(dict.Get(), "foo"));
    EXPECT_TRUE(ValidateDictAllResourcesOfType(dict.Get(), "bar"));

    // nullptr is not.
    EXPECT_FALSE(ValidateDictAllResourcesOfType(nullptr, "foo"));
    EXPECT_FALSE(ValidateDictAllResourcesOfType(nullptr, "bar"));

    // Add two correct dictionary entries and one string entry.
    auto new_dict = dict->SetNewFor<CPDF_Dictionary>("f1");
    new_dict->SetNewFor<CPDF_Name>("Type", "foo");
    new_dict = dict->SetNewFor<CPDF_Dictionary>("f2");
    new_dict->SetNewFor<CPDF_Name>("Type", "foo");
    dict->SetNewFor<CPDF_String>("f3", L"foo");
    EXPECT_FALSE(ValidateDictAllResourcesOfType(dict.Get(), "foo"));
    EXPECT_FALSE(ValidateDictAllResourcesOfType(dict.Get(), "bar"));

    // Change the string entry to a dictionary, but with the wrong /Type.
    new_dict = dict->SetNewFor<CPDF_Dictionary>("f3");
    new_dict->SetNewFor<CPDF_Name>("Type", "bar");
    EXPECT_FALSE(ValidateDictAllResourcesOfType(dict.Get(), "foo"));
    EXPECT_FALSE(ValidateDictAllResourcesOfType(dict.Get(), "bar"));

    // Change the /Type to match.
    new_dict->SetNewFor<CPDF_Name>("Type", "foo");
    EXPECT_TRUE(ValidateDictAllResourcesOfType(dict.Get(), "foo"));
    EXPECT_FALSE(ValidateDictAllResourcesOfType(dict.Get(), "bar"));
  }

  {
    // Indirect dictionary.
    auto doc = std::make_unique<CPDF_TestDocument>();
    auto dict = doc->New<CPDF_Dictionary>();

    // Add a correct dictionary entry.
    auto new_dict = doc->NewIndirect<CPDF_Dictionary>();
    new_dict->SetNewFor<CPDF_Name>("Type", "foo");
    dict->SetNewFor<CPDF_Reference>("f1", doc.get(), new_dict->GetObjNum());

    EXPECT_TRUE(ValidateDictAllResourcesOfType(dict.Get(), "foo"));
    EXPECT_FALSE(ValidateDictAllResourcesOfType(dict.Get(), "bar"));
  }

  DestroyPageModule();
}

TEST(ParserUtilityTest, ValidateDictOptionalType) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();

  // No type is ok.
  EXPECT_TRUE(ValidateDictOptionalType(dict.Get(), "foo"));
  EXPECT_TRUE(ValidateDictOptionalType(dict.Get(), "bar"));

  // Add the wrong object type.
  dict->SetNewFor<CPDF_String>("Type", L"foo");
  EXPECT_FALSE(ValidateDictOptionalType(dict.Get(), "foo"));
  EXPECT_FALSE(ValidateDictOptionalType(dict.Get(), "bar"));

  // Add the correct object type.
  dict->SetNewFor<CPDF_Name>("Type", "foo");
  EXPECT_TRUE(ValidateDictOptionalType(dict.Get(), "foo"));
  EXPECT_FALSE(ValidateDictOptionalType(dict.Get(), "bar"));
}

}  // namespace pdfium
