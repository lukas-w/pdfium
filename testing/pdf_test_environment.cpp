// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/pdf_test_environment.h"

#include <iostream>
#include <string>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "core/fxge/cfx_gemodule.h"

PDFTestEnvironment::PDFTestEnvironment() = default;

PDFTestEnvironment::~PDFTestEnvironment() = default;

// testing::Environment:
void PDFTestEnvironment::SetUp() {
  CFX_GEModule::Create(test_fonts_.FontPathsSpan(),
                       CFX_GEModule::RendererType::kDefault,
#if defined(PDF_ENABLE_FONTATIONS)
                       fontations_ ? CFX_FontMgr::FontBackend::kFontations
                                   : CFX_FontMgr::FontBackend::kFreeType);
#else
                       CFX_FontMgr::FontBackend::kFreeType);
#endif
}

void PDFTestEnvironment::TearDown() {
  CFX_GEModule::Destroy();
}

void PDFTestEnvironment::AddFlags(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    AddFlag(UNSAFE_TODO(argv[i]));
  }
}

void PDFTestEnvironment::AddFlag(const std::string& flag) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (flag == "--fontations") {
    fontations_ = true;
    return;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
  std::cerr << "Unknown flag: " << flag << "\n";
}
