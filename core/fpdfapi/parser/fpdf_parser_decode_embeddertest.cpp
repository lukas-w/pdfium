// Copyright 2015 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/fpdf_parser_decode.h"

#include "public/cpp/fpdf_scopers.h"
#include "testing/embedder_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_support.h"

using FPDFParserDecodeEmbedderTest = EmbedderTest;

TEST_F(FPDFParserDecodeEmbedderTest, Bug552046) {
  // Tests specifying multiple image filters for a stream. Should not cause a
  // crash when rendered and should produce a blank page.
  ASSERT_TRUE(OpenDocument("bug_552046.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()),
                     "blank_page_612x792");
}

TEST_F(FPDFParserDecodeEmbedderTest, Bug555784) {
  // Tests bad input to the run length decoder that caused a heap overflow.
  // Should not cause a crash when rendered. The image is 81915 pixels wide
  // with 3277 DeviceN components at 16 bits each, which is a valid but
  // enormous pitch, so its one scanline does get decoded and drawn.
  ASSERT_TRUE(OpenDocument("bug_555784.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()), "bug_555784");
}

TEST_F(FPDFParserDecodeEmbedderTest, Bug455199) {
  // Tests object numbers with a value > 01000000.
  // Should open successfully.
  ASSERT_TRUE(OpenDocument("bug_455199.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()), "hello_world");
}
