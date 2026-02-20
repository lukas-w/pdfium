// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/edit/cpdf_fontsubsetter.h"

#include <stdint.h>

#include <numeric>
#include <string>
#include <vector>

#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/retain_ptr.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/fx_string_testhelpers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

using ::testing::Contains;
using ::testing::IsEmpty;
using ::testing::Matcher;
using ::testing::UnorderedElementsAre;

namespace {

CPDF_Document* CPDFDocumentFromFPDFDocument(FPDF_DOCUMENT document) {
  // This is cheating slightly to avoid a layering violation, since this file
  // cannot include fpdfsdk/cpdfsdk_helpers.h to get access to
  // CPDFDocumentFromFPDFDocument().
  return reinterpret_cast<CPDF_Document*>(document);
}

// Returns the file path for a test font provided by the third-party test_fonts.
std::string GetTestFontFilePath(const std::string& file_name) {
  return PathService::GetThirdPartyFilePath("test_fonts/test_fonts/" +
                                            file_name);
}

// Returns a list of new object nums used for testing. Since the object nums
// list is used to see if an object num is new, this can exceed the amount of
// objects in the document. This uses a large enough number sufficient for these
// tests. As a result, tests can avoid having to explicitly write the new object
// numbers added when writing text to the document.
std::vector<uint32_t> GetTestNewObjNums() {
  std::vector<uint32_t> test_obj_nums(100);
  std::iota(test_obj_nums.begin(), test_obj_nums.end(), 1);
  return test_obj_nums;
}

// Matcher that verifies the stream size is strictly within the range min
// inclusive, max exclusive.
MATCHER_P2(StreamSizeIsWithinRange, min_size, max_size, "") {
  const auto& [obj_num, obj] = arg;

  if (!obj || !obj->IsStream()) {
    *result_listener << "is not a stream (obj_num: " << obj_num << ")";
    return false;
  }

  size_t actual_size = obj->AsStream()->GetRawSize();
  if (actual_size < min_size || actual_size >= max_size) {
    *result_listener << "stream size " << actual_size
                     << " is outside expected range (" << min_size << ", "
                     << max_size << ")";
    return false;
  }
  return true;
}

}  // namespace

// Prints overrides nicely for debugging purposes.
void PrintTo(const RetainPtr<const CPDF_Object>& obj, std::ostream* os) {
  if (!obj) {
    *os << "nullptr";
    return;
  }

  *os << "(Obj type=" << obj->GetType();
  if (obj->IsStream()) {
    *os << " size=" << obj->AsStream()->GetRawSize();
  }
  *os << ")";
}

class CPDFFontSubsetterTest : public EmbedderTest {
 public:
  void InsertNewTextObject(const std::wstring& text,
                           FPDF_PAGE page,
                           FPDF_FONT font) {
    FPDF_PAGEOBJECT text_object =
        FPDFPageObj_CreateTextObj(document(), font, 20.0f);
    EXPECT_TRUE(text_object);

    ScopedFPDFWideString fpdf_text = GetFPDFWideString(text);
    EXPECT_TRUE(FPDFText_SetText(text_object, fpdf_text.get()));

    const FS_MATRIX matrix{1.0f, 0.0f, 0.0f, 1.0f, 50.0f, 200.0f};
    ASSERT_TRUE(FPDFPageObj_TransformF(text_object, &matrix));
    FPDFPage_InsertObject(page, text_object);
    EXPECT_TRUE(FPDFPage_GenerateContent(page));
  }
};

TEST_F(CPDFFontSubsetterTest, NoNewText) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  CPDF_FontSubsetter subsetter(CPDFDocumentFromFPDFDocument(document()));

  EXPECT_THAT(subsetter.GenerateObjectOverrides({}), IsEmpty());

  EXPECT_THAT(subsetter.GenerateObjectOverrides(GetTestNewObjNums()),
              IsEmpty());

  // Not a text object.
  FPDF_PAGEOBJECT rect = FPDFPageObj_CreateNewRect(20, 100, 50, 50);
  FPDFPage_InsertObject(page.get(), rect);
  EXPECT_THAT(subsetter.GenerateObjectOverrides(GetTestNewObjNums()),
              IsEmpty());
}

TEST_F(CPDFFontSubsetterTest, OpenType) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  const std::string font_path =
      GetTestFontFilePath("NotoSansCJKjp-Regular.otf");
  ASSERT_FALSE(font_path.empty());

  std::vector<uint8_t> font_data = GetFileContents(font_path.c_str());
  const size_t original_size = font_data.size();
  ASSERT_EQ(16427228u, original_size);

  ScopedFPDFFont font(FPDFText_LoadFont(document(), font_data.data(),
                                        font_data.size(), FPDF_FONT_TRUETYPE,
                                        /*cid=*/true));
  ASSERT_TRUE(font);

  ASSERT_NO_FATAL_FAILURE(InsertNewTextObject(L"这", page.get(), font.get()));

  CPDF_FontSubsetter subsetter(CPDFDocumentFromFPDFDocument(document()));
  auto overrides = subsetter.GenerateObjectOverrides(GetTestNewObjNums());
  ASSERT_EQ(1u, overrides.size());

  // Subset size is ~2.5% of the original font file, i.e. ~450 KB.
  EXPECT_THAT(overrides, Contains(StreamSizeIsWithinRange(
                             original_size * 0.02, original_size * 0.03)));
}

TEST_F(CPDFFontSubsetterTest, TrueType) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  const std::string font_path = GetTestFontFilePath("Arimo-Regular.ttf");
  ASSERT_FALSE(font_path.empty());

  std::vector<uint8_t> font_data = GetFileContents(font_path.c_str());
  const size_t original_size = font_data.size();
  ASSERT_EQ(436180u, original_size);

  ScopedFPDFFont font(FPDFText_LoadFont(document(), font_data.data(),
                                        font_data.size(), FPDF_FONT_TRUETYPE,
                                        /*cid=*/true));
  ASSERT_TRUE(font);

  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Hello world", page.get(), font.get()));

  CPDF_FontSubsetter subsetter(CPDFDocumentFromFPDFDocument(document()));
  auto overrides = subsetter.GenerateObjectOverrides(GetTestNewObjNums());
  ASSERT_EQ(1u, overrides.size());

  // Subset size is ~3% of the original font file, i.e. ~13 KB.
  EXPECT_THAT(overrides, Contains(StreamSizeIsWithinRange(
                             original_size * 0.025, original_size * 0.035)));
}

TEST_F(CPDFFontSubsetterTest, SingleFontMultipleTexts) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  const std::string font_path = GetTestFontFilePath("Arimo-Regular.ttf");
  ASSERT_FALSE(font_path.empty());

  std::vector<uint8_t> font_data = GetFileContents(font_path.c_str());
  const size_t original_size = font_data.size();
  ASSERT_EQ(436180u, original_size);

  ScopedFPDFFont font(FPDFText_LoadFont(document(), font_data.data(),
                                        font_data.size(), FPDF_FONT_TRUETYPE,
                                        /*cid=*/true));
  ASSERT_TRUE(font);

  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Abcdefg", page.get(), font.get()));
  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Hijklmnop", page.get(), font.get()));

  CPDF_FontSubsetter subsetter(CPDFDocumentFromFPDFDocument(document()));
  auto overrides = subsetter.GenerateObjectOverrides(GetTestNewObjNums());
  ASSERT_EQ(1u, overrides.size());

  // Subset size is ~3.5% of the original font file, i.e. ~15 KB.
  EXPECT_THAT(overrides, Contains(StreamSizeIsWithinRange(
                             original_size * 0.03, original_size * 0.04)));
}

TEST_F(CPDFFontSubsetterTest, MultipleFontsMultipleTexts) {
  CreateEmptyDocument();
  ScopedFPDFPage page(FPDFPage_New(document(), 0, 400, 400));

  const std::string font_path1 = GetTestFontFilePath("Lohit-Tamil.ttf");
  ASSERT_FALSE(font_path1.empty());
  const std::string font_path2 = GetTestFontFilePath("Arimo-Regular.ttf");
  ASSERT_FALSE(font_path2.empty());

  std::vector<uint8_t> font_data1 = GetFileContents(font_path1.c_str());
  const size_t original_size1 = font_data1.size();
  ASSERT_EQ(48908u, original_size1);
  std::vector<uint8_t> font_data2 = GetFileContents(font_path2.c_str());
  const size_t original_size2 = font_data2.size();
  ASSERT_EQ(436180u, original_size2);

  ScopedFPDFFont font1(FPDFText_LoadFont(document(), font_data1.data(),
                                         font_data1.size(), FPDF_FONT_TRUETYPE,
                                         /*cid=*/true));
  ASSERT_TRUE(font1);
  ScopedFPDFFont font2(FPDFText_LoadFont(document(), font_data2.data(),
                                         font_data2.size(), FPDF_FONT_TRUETYPE,
                                         /*cid=*/true));
  ASSERT_TRUE(font2);

  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"வணக்கம்", page.get(), font1.get()));
  ASSERT_NO_FATAL_FAILURE(
      InsertNewTextObject(L"Goodbye", page.get(), font2.get()));

  CPDF_FontSubsetter subsetter(CPDFDocumentFromFPDFDocument(document()));
  auto overrides = subsetter.GenerateObjectOverrides(GetTestNewObjNums());
  ASSERT_EQ(2u, overrides.size());

  // Subset size for `font_data1` is ~6% of the original file, i.e. ~3 KB.
  // Subset size for `font_data2` is ~3% of the original file, i.e. ~13.3 KB.
  EXPECT_THAT(overrides, UnorderedElementsAre(
                             StreamSizeIsWithinRange(original_size1 * 0.055,
                                                     original_size1 * 0.065),
                             StreamSizeIsWithinRange(original_size2 * 0.025,
                                                     original_size2 * 0.035)));
}
