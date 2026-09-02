// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_PDF_TEST_ENVIRONMENT_H_
#define TESTING_PDF_TEST_ENVIRONMENT_H_

#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/test_fonts.h"

class PDFTestEnvironment : public testing::Environment {
 public:
  PDFTestEnvironment();
  ~PDFTestEnvironment() override;

  // testing::Environment:
  void SetUp() override;
  void TearDown() override;

  void AddFlags(int argc, char** argv);

 private:
  void AddFlag(const std::string& flag);

#if defined(PDF_ENABLE_FONTATIONS)
  bool fontations_ = false;
#endif  // defined(PDF_ENABLE_FONTATIONS)
  TestFonts test_fonts_;
};

#endif  // TESTING_PDF_TEST_ENVIRONMENT_H_
