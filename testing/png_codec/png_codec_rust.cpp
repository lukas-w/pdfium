// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/png_codec/png_codec.h"

#include <stdint.h>

#include <vector>

#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/span_util.h"
#include "testing/png_codec/png_codec_rust_ffi.rs.h"

namespace png_codec {

std::vector<uint8_t> Decode(pdfium::span<const uint8_t> input,
                            bool reverse_byte_order,
                            int* width,
                            int* height) {
  rust::Slice<const uint8_t> src_slice(input);
  int32_t w = 0;
  int32_t h = 0;
  rust::Vec<uint8_t> decoded =
      rust_png::decode_png(src_slice, reverse_byte_order, w, h);
  if (w <= 0 || h <= 0 || decoded.empty()) {
    return {};
  }
  *width = w;
  *height = h;
  return {decoded.begin(), decoded.end()};
}

std::vector<uint8_t> EncodeBGR(pdfium::span<const uint8_t> input,
                               int width,
                               int height,
                               int row_byte_width) {
  rust::Slice<const uint8_t> src_slice(input);
  rust::Vec<uint8_t> encoded = rust_png::encode_bgr(
      src_slice, width, height, pdfium::checked_cast<size_t>(row_byte_width));
  if (encoded.empty()) {
    return {};
  }
  return {encoded.begin(), encoded.end()};
}

std::vector<uint8_t> EncodeRGBA(pdfium::span<const uint8_t> input,
                                int width,
                                int height,
                                int row_byte_width) {
  rust::Slice<const uint8_t> src_slice(input);
  rust::Vec<uint8_t> encoded = rust_png::encode_rgba(
      src_slice, width, height, pdfium::checked_cast<size_t>(row_byte_width));
  if (encoded.empty()) {
    return {};
  }
  return {encoded.begin(), encoded.end()};
}

std::vector<uint8_t> EncodeBGRA(pdfium::span<const uint8_t> input,
                                int width,
                                int height,
                                int row_byte_width,
                                bool discard_transparency) {
  rust::Slice<const uint8_t> src_slice(input);
  rust::Vec<uint8_t> encoded = rust_png::encode_bgra(
      src_slice, width, height, pdfium::checked_cast<size_t>(row_byte_width),
      discard_transparency);
  if (encoded.empty()) {
    return {};
  }
  return {encoded.begin(), encoded.end()};
}

std::vector<uint8_t> EncodeGray(pdfium::span<const uint8_t> input,
                                int width,
                                int height,
                                int row_byte_width) {
  rust::Slice<const uint8_t> src_slice(input);
  rust::Vec<uint8_t> encoded = rust_png::encode_gray(
      src_slice, width, height, pdfium::checked_cast<size_t>(row_byte_width));
  if (encoded.empty()) {
    return {};
  }
  return {encoded.begin(), encoded.end()};
}

}  // namespace png_codec
