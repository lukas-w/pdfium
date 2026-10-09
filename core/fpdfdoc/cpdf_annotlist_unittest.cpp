// Copyright 2023 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpdf_annotlist.h"

#include <stdint.h>

#include <initializer_list>
#include <memory>

#include "constants/annotation_common.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/test_with_page_module.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fpdfapi/render/cpdf_rendercontext.h"
#include "core/fpdfdoc/cpdf_annot.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/widestring.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class CPDFAnnotListTest : public TestWithPageModule {
 public:
  void SetUp() override {
    TestWithPageModule::SetUp();

    document_ = std::make_unique<CPDF_TestDocument>();
    document_->SetRoot(pdfium::MakeRetain<CPDF_Dictionary>());
    page_ = pdfium::MakeRetain<CPDF_Page>(
        document_.get(), pdfium::MakeRetain<CPDF_Dictionary>());
  }

  void TearDown() override {
    page_.Reset();
    document_.reset();

    TestWithPageModule::TearDown();
  }

 protected:
  void AddTextAnnotation(const ByteString& contents) {
    RetainPtr<CPDF_Dictionary> annotation =
        page_->GetOrCreateAnnotsArray()->AppendNew<CPDF_Dictionary>();
    annotation->SetNewFor<CPDF_Name>(pdfium::annotation::kSubtype, "Text");
    annotation->SetNewFor<CPDF_String>(pdfium::annotation::kContents, contents);
  }

  std::unique_ptr<CPDF_TestDocument> document_;
  RetainPtr<CPDF_Page> page_;
};

ByteString MakeByteString(std::initializer_list<uint8_t> bytes) {
  // SAFETY: compiler determines size of initializer_list.
  return UNSAFE_BUFFERS(ByteString(std::data(bytes), std::size(bytes)));
}

ByteString GetRawContents(const CPDF_Annot* annotation) {
  return annotation->GetAnnotDict()->GetByteStringFor(
      pdfium::annotation::kContents);
}

WideString GetDecodedContents(const CPDF_Annot* annotation) {
  return annotation->GetAnnotDict()->GetUnicodeTextFor(
      pdfium::annotation::kContents);
}

FX_ARGB GetCenterPixel(const CFX_DIBitmap* bitmap) {
  return FXARGB_GetDIB(bitmap->GetScanline(100).subspan<400, 4>());
}

}  // namespace

TEST_F(CPDFAnnotListTest, CreatePopupAnnotFromPdfEncoded) {
  const ByteString kContents = MakeByteString({'A', 'a', 0xE4, 0xA0});
  AddTextAnnotation(kContents);

  CPDF_AnnotList list(page_);

  ASSERT_EQ(2u, list.Count());
  EXPECT_EQ(kContents, GetRawContents(list.GetAt(1)));
  EXPECT_EQ(WideString::FromUTF8("Aaä€"), GetDecodedContents(list.GetAt(1)));
}

TEST_F(CPDFAnnotListTest, CreatePopupAnnotFromUnicode) {
  const ByteString kContents =
      MakeByteString({0xFE, 0xFF, 0x00, 'A', 0x00, 'a', 0x00, 0xE4, 0x20, 0xAC,
                      0xD8, 0x3C, 0xDF, 0xA8});
  AddTextAnnotation(kContents);

  CPDF_AnnotList list(page_);

  ASSERT_EQ(2u, list.Count());
  EXPECT_EQ(kContents, GetRawContents(list.GetAt(1)));

  EXPECT_EQ(WideString::FromUTF8("Aaä€🎨"), GetDecodedContents(list.GetAt(1)));
}

TEST_F(CPDFAnnotListTest, CreatePopupAnnotFromEmptyPdfEncoded) {
  AddTextAnnotation("");

  CPDF_AnnotList list(page_);

  EXPECT_EQ(1u, list.Count());
}

TEST_F(CPDFAnnotListTest, CreatePopupAnnotFromEmptyUnicode) {
  const ByteString kContents = MakeByteString({0xFE, 0xFF});
  AddTextAnnotation(kContents);

  CPDF_AnnotList list(page_);

  EXPECT_EQ(1u, list.Count());
}

TEST_F(CPDFAnnotListTest, CreatePopupAnnotFromEmptyUnicodedWithEscape) {
  const ByteString kContents =
      MakeByteString({0xFE, 0xFF, 0x00, 0x1B, 'j', 'a', 0x00, 0x1B});
  AddTextAnnotation(kContents);

  CPDF_AnnotList list(page_);

  EXPECT_EQ(1u, list.Count());
}

TEST_F(CPDFAnnotListTest, ClosedPopupAppearanceObjects) {
  AddTextAnnotation("Regression comment");
  {
    CPDF_AnnotList list(page_);
    ASSERT_EQ(2u, list.Count());
    EXPECT_TRUE(list.GetAt(0)->GetAnnotDict()->KeyExist("AP"));
    EXPECT_FALSE(list.GetAt(1)->GetAnnotDict()->KeyExist("AP"));
  }
  const uint32_t object_count = document_->GetLastObjNum();
  for (int i = 0; i < 30; ++i) {
    CPDF_AnnotList list(page_);
    EXPECT_FALSE(list.GetAt(1)->GetAnnotDict()->KeyExist("AP"));
    EXPECT_EQ(object_count, document_->GetLastObjNum());
  }
}

class CPDFPopupRenderTest : public CPDFAnnotListTest,
                            public testing::WithParamInterface<bool> {
 protected:
  bool UseRenderContext() const { return GetParam(); }

  bool DrawPopup(CPDF_Annot* popup, CFX_RenderDevice* device) {
    device->Clear(0xffffffff);
    const CFX_Matrix matrix;
    if (!UseRenderContext()) {
      return popup->DrawAppearance(page_.Get(), device, matrix,
                                   CPDF_Annot::AppearanceMode::kNormal);
    }
    CPDF_RenderContext context(document_.get(), nullptr, nullptr);
    const bool result = popup->DrawInContext(
        page_.Get(), &context, matrix, CPDF_Annot::AppearanceMode::kNormal);
    context.Render(device, nullptr, nullptr, nullptr);
    return result;
  }
};

TEST_P(CPDFPopupRenderTest, PopupRendering) {
  AddTextAnnotation("Regression comment");
  CPDF_AnnotList list(page_);
  ASSERT_EQ(2u, list.Count());
  CPDF_Annot* popup = list.GetAt(1);
  ASSERT_EQ(CPDF_Annot::Subtype::POPUP, popup->GetSubtype());
  EXPECT_FALSE(popup->GetAnnotDict()->KeyExist("AP"));
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(256, 256, FXDIB_Format::kBgra));
  auto device = CFX_RenderDevice::CreateForBitmap(bitmap);
  ASSERT_TRUE(device);
  EXPECT_FALSE(DrawPopup(popup, device.get()));
  EXPECT_EQ(0xffffffffu, GetCenterPixel(bitmap.Get()));
  popup->SetOpenState(true);
  EXPECT_TRUE(DrawPopup(popup, device.get()));
  EXPECT_EQ(0xffffff00u, GetCenterPixel(bitmap.Get()));
  ASSERT_TRUE(popup->GetAnnotDict()->KeyExist("AP"));
  const uint32_t object_count = document_->GetLastObjNum();
  popup->SetOpenState(false);
  EXPECT_FALSE(DrawPopup(popup, device.get()));
  EXPECT_EQ(0xffffffffu, GetCenterPixel(bitmap.Get()));
  popup->SetOpenState(true);
  EXPECT_TRUE(DrawPopup(popup, device.get()));
  EXPECT_EQ(0xffffff00u, GetCenterPixel(bitmap.Get()));
  EXPECT_EQ(object_count, document_->GetLastObjNum());
}

INSTANTIATE_TEST_SUITE_P(All, CPDFPopupRenderTest, testing::Bool());
