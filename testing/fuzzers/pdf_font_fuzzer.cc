// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/span.h"
#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"

static constexpr size_t kMaxFuzzBytes = 1024 * 1024 * 1024;  // 1 GB.

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 2 || size > kMaxFuzzBytes) {
    return 0;
  }

  // SAFETY: required from fuzzer API.
  auto [pre, span] = UNSAFE_BUFFERS(pdfium::span(data, size)).split_at<2u>();
  int font_type = pre[0];
  FPDF_BOOL cid = pre[1];

  ScopedFPDFDocument doc(FPDF_CreateNewDocument());
  ScopedFPDFPage page(FPDFPage_New(doc.get(), 0, 612, 792));
  ScopedFPDFFont font(FPDFText_LoadFont(doc.get(), span.data(),
                                        static_cast<uint32_t>(span.size()),
                                        font_type, cid));
  if (!font) {
    return 0;
  }

  FPDF_PAGEOBJECT text_object =
      FPDFPageObj_CreateTextObj(doc.get(), font.get(), 12.0f);
  FPDFPage_InsertObject(page.get(), text_object);
  FPDFPage_GenerateContent(page.get());
  return 0;
}
