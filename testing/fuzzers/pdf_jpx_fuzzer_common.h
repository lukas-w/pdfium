// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_FUZZERS_PDF_JPX_FUZZER_UTILS_H_
#define TESTING_FUZZERS_PDF_JPX_FUZZER_UTILS_H_

#include <stdint.h>

#include "core/fxcodec/jpx/cjpx_decoder.h"
#include "core/fxcrt/span.h"

// Decodes `data` as a JPEG 2000 image with the given decoder parameters,
// bailing out early on images that are too large to decode in a fuzzer.
void DecodeJpxForFuzzing(pdfium::span<const uint8_t> data,
                         CJPX_Decoder::ColorSpaceOption color_space_option,
                         uint8_t resolution_levels_to_skip,
                         bool strict_mode);

#endif  // TESTING_FUZZERS_PDF_JPX_FUZZER_UTILS_H_
