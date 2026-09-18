// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"

#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fxjs/ijs_runtime.h"
#include "public/fpdfview.h"
#include "testing/embedder_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/v8_test_environment.h"
#include "testing/xfa_js_embedder_test.h"

class CPDFXFAContextEmbedderTest : public XFAJSEmbedderTest {};

// Should not crash. Uses external unattached isolate from test environment.
TEST_F(CPDFXFAContextEmbedderTest, HasHeap) {
  ASSERT_FALSE(IJS_Runtime::IsIsolatePerDocument());
  ASSERT_TRUE(OpenDocument("simple_xfa.pdf"));

  CPDF_Document* pDocument = CPDFDocumentFromFPDFDocument(document());
  auto* context = static_cast<CPDFXFA_Context*>(pDocument->GetExtension());
  ASSERT_TRUE(context);
  EXPECT_FALSE(context->GetIsolate());
  ASSERT_TRUE(context->GetGCHeap());
  EXPECT_FALSE(context->GetGCHeap()->GetAttachedHeap());
  EXPECT_TRUE(context->GetGCHeap()->GetStandaloneHeap());
}

TEST_F(CPDFXFAContextEmbedderTest, UnattachedPDFiumCreatedIsolate) {
  FPDF_DestroyLibrary();

  const FPDF_LIBRARY_CONFIG kConfig = {
      .version = 7,
      .m_pUserFontPaths = nullptr,
      .m_pIsolate = nullptr,
      .m_v8EmbedderSlot = 0,
      .m_pPlatform = V8TestEnvironment::GetInstance()->platform(),
      .m_RendererType = FPDF_RENDERERTYPE_AGG,
      .m_FontLibraryType = FPDF_FONTBACKENDTYPE_FREETYPE,
      .m_BrotliEnabled = false,
      .m_IsolatePerDocument = false,
  };
  FPDF_InitLibraryWithConfig(&kConfig);
  ASSERT_FALSE(IJS_Runtime::IsIsolatePerDocument());

  ASSERT_TRUE(OpenDocument("simple_xfa.pdf"));

  CPDF_Document* pDocument = CPDFDocumentFromFPDFDocument(document());
  auto* context = static_cast<CPDFXFA_Context*>(pDocument->GetExtension());
  ASSERT_TRUE(context);
  EXPECT_FALSE(context->GetIsolate());
  ASSERT_TRUE(context->GetGCHeap());
  EXPECT_FALSE(context->GetGCHeap()->GetAttachedHeap());
  EXPECT_TRUE(context->GetGCHeap()->GetStandaloneHeap());

  CloseDocument();

  FPDF_DestroyLibrary();

  EmbedderTestEnvironment::GetInstance()->TearDown();
  EmbedderTestEnvironment::GetInstance()->SetUp();
}

TEST_F(CPDFXFAContextEmbedderTest, IsolatePerDocument) {
  FPDF_DestroyLibrary();

  const FPDF_LIBRARY_CONFIG kConfig = {
      .version = 7,
      .m_pUserFontPaths = nullptr,
      .m_pIsolate = nullptr,
      .m_v8EmbedderSlot = 0,
      .m_pPlatform = V8TestEnvironment::GetInstance()->platform(),
      .m_RendererType = FPDF_RENDERERTYPE_AGG,
      .m_FontLibraryType = FPDF_FONTBACKENDTYPE_FREETYPE,
      .m_BrotliEnabled = false,
      .m_IsolatePerDocument = true,
  };
  FPDF_InitLibraryWithConfig(&kConfig);
  ASSERT_TRUE(IJS_Runtime::IsIsolatePerDocument());

  ASSERT_TRUE(OpenDocument("simple_xfa.pdf"));

  CPDF_Document* pDocument = CPDFDocumentFromFPDFDocument(document());
  auto* context = static_cast<CPDFXFA_Context*>(pDocument->GetExtension());
  ASSERT_TRUE(context);
  EXPECT_TRUE(context->GetIsolate());
  ASSERT_TRUE(context->GetGCHeap());
  EXPECT_TRUE(context->GetGCHeap()->GetAttachedHeap());
  EXPECT_FALSE(context->GetGCHeap()->GetStandaloneHeap());

  CloseDocument();

  FPDF_DestroyLibrary();

  EmbedderTestEnvironment::GetInstance()->TearDown();
  EmbedderTestEnvironment::GetInstance()->SetUp();
}
