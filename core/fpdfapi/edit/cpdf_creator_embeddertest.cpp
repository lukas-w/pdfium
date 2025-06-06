// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string.h>

#include <string>

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/fake_file_access.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"

class CPDFCreatorEmbedderTest : public EmbedderTest {};

TEST_F(CPDFCreatorEmbedderTest, SavedDocsAreEqualAfterParse) {
  ASSERT_TRUE(OpenDocument("annotation_stamp_with_ap.pdf"));
  // Save without additional data reading.
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  const std::string saved_doc_1 = GetString();
  ClearString();

  {
    // Do some read only operations.
    ASSERT_GE(1, FPDF_GetPageCount(document()));
    ScopedPage page = LoadScopedPage(0);
    ASSERT_TRUE(page);
    ScopedFPDFBitmap bitmap = RenderLoadedPageWithFlags(page.get(), FPDF_ANNOT);
    EXPECT_EQ(595, FPDFBitmap_GetWidth(bitmap.get()));
    EXPECT_EQ(842, FPDFBitmap_GetHeight(bitmap.get()));
  }

  // Save when we have additional loaded data.
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  const std::string saved_doc_2 = GetString();
  ClearString();

  // The sizes of saved docs should be equal.
  EXPECT_EQ(saved_doc_1.size(), saved_doc_2.size());
}

TEST_F(CPDFCreatorEmbedderTest, Bug873) {
  ASSERT_TRUE(OpenDocument("embedded_attachments.pdf"));
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

  // Cannot match second part of the ID since it is randomly generated.
  std::string saved_data = GetString();
  const char kTrailerBeforeSecondID[] =
      "trailer\r\n<</Info 9 0 R /Root 11 0 R /Size "
      "36/ID[<D889EB6B9ADF88E5EDA7DC08FE85978B><";
  ASSERT_THAT(saved_data, testing::HasSubstr(kTrailerBeforeSecondID));
  size_t trailer_start = saved_data.find(kTrailerBeforeSecondID);
  static constexpr size_t kIdLen = 32;
  size_t trailer_continuation =
      trailer_start + UNSAFE_TODO(strlen(kTrailerBeforeSecondID)) + kIdLen;
  std::string data_after_second_id = saved_data.substr(trailer_continuation);
  EXPECT_THAT(data_after_second_id, testing::StartsWith(">]>>\r\n"));
}

TEST_F(CPDFCreatorEmbedderTest, SaveLinearizedInfo) {
  FileAccessForTesting file_acc("linearized.pdf");
  FakeFileAccess fake_acc(&file_acc);

  CreateAvail(fake_acc.GetFileAvail(), fake_acc.GetFileAccess());
  while (PDF_DATA_AVAIL !=
         FPDFAvail_IsDocAvail(avail(), fake_acc.GetDownloadHints())) {
    fake_acc.SetRequestedDataAvailable();
  }

  SetDocumentFromAvail();
  ASSERT_TRUE(document());

  // Load second page, to parse additional crossref sections.
  while (PDF_DATA_AVAIL !=
         FPDFAvail_IsPageAvail(avail(), 1, fake_acc.GetDownloadHints())) {
    fake_acc.SetRequestedDataAvailable();
  }
  // Simulate downloading of whole file.
  fake_acc.SetWholeFileAvailable();
  // Save document.
  EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));
  const std::string saved_doc = GetString();

  EXPECT_THAT(saved_doc, ::testing::HasSubstr("/Info"));
}
