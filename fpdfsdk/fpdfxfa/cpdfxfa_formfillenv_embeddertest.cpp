// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "fxjs/ijs_runtime.h"
#include "public/fpdf_formfill.h"
#include "public/fpdfview.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/path_service.h"
#include "testing/xfa_js_embedder_test.h"

class CPDFXFAFormFillEnvEmbedderTest : public XFAJSEmbedderTest {};

TEST_F(CPDFXFAFormFillEnvEmbedderTest, ReinitFormFillEnvironment) {
  ASSERT_FALSE(IJS_Runtime::IsIsolatePerDocument());

  std::string file_path = PathService::GetTestFilePath("simple_xfa.pdf");
  ScopedFPDFDocument doc(FPDF_LoadDocument(file_path.c_str(), ""));
  ASSERT_TRUE(doc);

  IPDF_JSPLATFORM platform = {};
  platform.version = 3;

  FPDF_FORMFILLINFO formfillinfo = {};
  formfillinfo.version = 2;
  formfillinfo.m_pJsPlatform = &platform;

  ScopedFPDFFormHandle form(
      FPDFDOC_InitFormFillEnvironment(doc.get(), &formfillinfo));
  ASSERT_TRUE(form);
  EXPECT_TRUE(FPDF_LoadXFA(doc.get()));

  form.reset();

  form.reset(FPDFDOC_InitFormFillEnvironment(doc.get(), &formfillinfo));
  ASSERT_TRUE(form);
  EXPECT_TRUE(FPDF_LoadXFA(doc.get()));

  ScopedFPDFPage page(FPDF_LoadPage(doc.get(), 0));
  EXPECT_TRUE(page);
}

TEST_F(CPDFXFAFormFillEnvEmbedderTest, CloseDocumentBeforeExit) {
  ASSERT_FALSE(IJS_Runtime::IsIsolatePerDocument());

  std::string file_path = PathService::GetTestFilePath("simple_xfa.pdf");
  ScopedFPDFDocument doc(FPDF_LoadDocument(file_path.c_str(), ""));
  ASSERT_TRUE(doc);

  IPDF_JSPLATFORM platform = {};
  platform.version = 3;

  FPDF_FORMFILLINFO formfillinfo = {};
  formfillinfo.version = 2;
  formfillinfo.m_pJsPlatform = &platform;

  ScopedFPDFFormHandle form(
      FPDFDOC_InitFormFillEnvironment(doc.get(), &formfillinfo));
  ASSERT_TRUE(form);
  EXPECT_TRUE(FPDF_LoadXFA(doc.get()));

  doc.reset();
  form.reset();
}
