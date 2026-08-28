// Copyright 2013 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_PNG_CODEC_PNG_CODEC_H_
#define TESTING_PNG_CODEC_PNG_CODEC_H_

#include <stdint.h>

#include <vector>

#include "core/fxcrt/span.h"

namespace png_codec {

// Decode a PNG into an RGBA pixel array, or BGRA pixel array if
// |reverse_byte_order| is set to true.
std::vector<uint8_t> Decode(pdfium::span<const uint8_t> input,
                            bool reverse_byte_order,
                            int* width,
                            int* height);

// Encode a BGR pixel array into a PNG.
std::vector<uint8_t> EncodeBGR(pdfium::span<const uint8_t> input,
                               int width,
                               int height,
                               int row_byte_width);

// Encode an RGBA pixel array into a PNG.
std::vector<uint8_t> EncodeRGBA(pdfium::span<const uint8_t> input,
                                int width,
                                int height,
                                int row_byte_width);

// Encode an BGRA pixel array into a PNG.
std::vector<uint8_t> EncodeBGRA(pdfium::span<const uint8_t> input,
                                int width,
                                int height,
                                int row_byte_width,
                                bool discard_transparency);

// Encode a grayscale pixel array into a PNG.
std::vector<uint8_t> EncodeGray(pdfium::span<const uint8_t> input,
                                int width,
                                int height,
                                int row_byte_width);

}  // namespace png_codec

#endif  // TESTING_PNG_CODEC_PNG_CODEC_H_
