// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdint>

#include "core/fxcodec/jpx/cjpx_decoder.h"
#include "core/fxcrt/span.h"
#include "testing/fuzzers/pdf_jpx_fuzzer_common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 3) {
    return 0;
  }

  // SAFETY: trusted arguments from fuzzer.
  auto [pre, span] = UNSAFE_BUFFERS(pdfium::span(data, size)).split_at<3u>();

  auto color_space_option =
      static_cast<CJPX_Decoder::ColorSpaceOption>(pre[0] % 3);
  uint8_t resolution_levels_to_skip = pre[1];
  bool strict_mode = !!pre[2];

  DecodeJpxForFuzzing(span, color_space_option, resolution_levels_to_skip,
                      strict_mode);
  return 0;
}
