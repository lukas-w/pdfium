// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class BrotliEnabledEmbedderTest : public EmbedderTest {
 public:
  virtual bool GetBrotliEnabled() { return true; }
  virtual int GetVersionSelected() { return 6; }

 private:
  void SetUp() override {
    auto* env = EmbedderTestEnvironment::GetInstance();
    previous_brotli_ = env->GetBrotli();
    previous_version_ = env->GetVersion();
    env->TearDown();
    env->SetBrotli(GetBrotliEnabled());
    env->SetVersion(GetVersionSelected());
    env->SetUp();
    EmbedderTest::SetUp();
  }

  void TearDown() override {
    EmbedderTest::TearDown();
    auto* env = EmbedderTestEnvironment::GetInstance();
    env->TearDown();
    env->SetBrotli(previous_brotli_);
    env->SetVersion(previous_version_);
    env->SetUp();
  }

  bool previous_brotli_;
  int previous_version_;
};

class BrotliDisabledEmbedderTest : public BrotliEnabledEmbedderTest {
 public:
  bool GetBrotliEnabled() override { return false; }
};

class BrotliV5EmbedderTest : public BrotliEnabledEmbedderTest {
 public:
  int GetVersionSelected() override { return 5; }
};

TEST_F(BrotliEnabledEmbedderTest, ManyRectanglesBrotli) {
  ASSERT_TRUE(OpenDocument("many_rectangles_brotli.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()),
                     "many_rectangles");
}

TEST_F(BrotliEnabledEmbedderTest, SimpleBrotliWithText) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()), "hello_world");
}

TEST_F(BrotliEnabledEmbedderTest, BrotliRectangles) {
  ASSERT_TRUE(OpenDocument("rectangles_brotli.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()), "rectangles");
}

TEST_F(BrotliEnabledEmbedderTest, BrotliDecodeParms) {
  ASSERT_TRUE(OpenDocument("brotli_decodeparms.pdf"));

  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);

  ScopedFPDFBitmap bitmap = RenderLoadedPage(page.get());
  ASSERT_TRUE(bitmap);

  CompareBitmap(bitmap.get(), "brotli_decode_parms");
}

TEST_F(BrotliEnabledEmbedderTest, BrotliWithLength1Argument) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli_with_length1.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()), "hello_world");
}

TEST_F(BrotliDisabledEmbedderTest, BrotliDecodeDisabled) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()),
                     "blank_page_200x200");
}

TEST_F(BrotliV5EmbedderTest, WrongBrotliVersion) {
  ASSERT_TRUE(OpenDocument("hello_world_brotli.pdf"));
  ScopedPage page = LoadScopedPage(0);
  ASSERT_TRUE(page);
  CompareDisplayList(RenderLoadedPageToDisplayList(page.get()),
                     "blank_page_200x200");
}
