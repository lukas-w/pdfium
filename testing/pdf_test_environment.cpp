// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/pdf_test_environment.h"

#include <iostream>
#include <string>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "core/fxge/cfx_gemodule.h"

namespace {

CFX_GEModule::RendererType RendererTypeForTest() {
#if defined(PDF_USE_SKIA)
  return CFX_GEModule::RendererType::kSkia;
#elif defined(PDF_USE_AGG)
  return CFX_GEModule::RendererType::kAgg;
#else
#error "Define PDF_USE_AGG or PDF_USE_SKIA in the build env"
#endif
}

CFX_GEModule::FontBackend FontBackendForTest([[maybe_unused]] bool fontations) {
#if defined(PDF_ENABLE_FONTATIONS) && defined(PDF_ENABLE_FREETYPE)
  return fontations ? CFX_GEModule::FontBackend::kFontations
                    : CFX_GEModule::FontBackend::kFreeType;
#elif defined(PDF_ENABLE_FONTATIONS)
  return CFX_GEModule::FontBackend::kFontations;
#elif defined(PDF_ENABLE_FREETYPE)
  return CFX_GEModule::FontBackend::kFreeType;
#else
#error "Define PDF_ENABLE_FREETYPE or PDF_ENABLE_FONTATIONS in the build env"
#endif
}

}  // namespace

PDFTestEnvironment::PDFTestEnvironment() = default;

PDFTestEnvironment::~PDFTestEnvironment() = default;

// testing::Environment:
void PDFTestEnvironment::SetUp() {
#if defined(PDF_ENABLE_FONTATIONS)
  const bool fontations = fontations_;
#else
  const bool fontations = false;
#endif
  CFX_GEModule::Create(test_fonts_.FontPathsSpan(), RendererTypeForTest(),
                       FontBackendForTest(fontations));
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
