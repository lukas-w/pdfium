// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/ijs_runtime.h"

#include "testing/gtest/include/gtest/gtest.h"

class IJSRuntimeTest : public testing::Test {
 public:
  void TearDown() override {
    IJS_Runtime::Destroy();
    testing::Test::TearDown();
  }
};

TEST_F(IJSRuntimeTest, IsolatePerDocument) {
  EXPECT_FALSE(IJS_Runtime::IsIsolatePerDocument());

  IJS_Runtime::Initialize(0, nullptr, nullptr, /*isolate_per_document=*/true);
  EXPECT_TRUE(IJS_Runtime::IsIsolatePerDocument());

  IJS_Runtime::Destroy();
  EXPECT_FALSE(IJS_Runtime::IsIsolatePerDocument());

  IJS_Runtime::Initialize(0, nullptr, nullptr, /*isolate_per_document=*/false);
  EXPECT_FALSE(IJS_Runtime::IsIsolatePerDocument());

  IJS_Runtime::Destroy();
  EXPECT_FALSE(IJS_Runtime::IsIsolatePerDocument());
}
