// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/brotli/brotli_decoder.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "core/fxcodec/image_predictors.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "third_party/brotli/include/brotli/decode.h"

namespace {

constexpr size_t kMaxDecodeBytes = 1024 * 1024 * 1024;
bool g_brotli_enabled = false;

struct BrotliDecoderStateDeleter {
  void operator()(struct BrotliDecoderStateStruct* ptr) const {
    BrotliDecoderDestroyInstance(ptr);
  }
};

}  // namespace

DataAndBytesConsumed BrotliDecoder::Decode(pdfium::span<const uint8_t> src_span,
                                           int predictor_value,
                                           int colors,
                                           int bits_per_component,
                                           int columns,
                                           uint32_t estimated_decode_size) {
  CHECK(g_brotli_enabled);
  if (src_span.empty()) {
    return {DataVector<uint8_t>(), 0u};
  }
  if (estimated_decode_size == 0) {
    estimated_decode_size = pdfium::checked_cast<uint32_t>(src_span.size());
  }
  if (estimated_decode_size > kMaxDecodeBytes) {
    return {DataVector<uint8_t>(), 0u};
  }
  std::unique_ptr<BrotliDecoderState, BrotliDecoderStateDeleter> state(
      BrotliDecoderCreateInstance(nullptr, nullptr, nullptr));
  if (!state) {
    return {DataVector<uint8_t>(), 0u};
  }
  BrotliDecoderSetParameter(state.get(), BROTLI_DECODER_PARAM_LARGE_WINDOW, 1u);

  DataVector<uint8_t> decoded_buffer(estimated_decode_size);
  size_t available_in = src_span.size();
  const uint8_t* next_in = src_span.data();
  size_t total_out = 0;

  while (true) {
    auto output_span = pdfium::span(decoded_buffer).subspan(total_out);
    size_t available_out = output_span.size();
    uint8_t* next_out = output_span.data();
    BrotliDecoderResult result =
        BrotliDecoderDecompressStream(state.get(), &available_in, &next_in,
                                      &available_out, &next_out, &total_out);
    if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT &&
        decoded_buffer.size() < kMaxDecodeBytes) {
      decoded_buffer.resize(
          std::min(decoded_buffer.size() * 2, kMaxDecodeBytes));
      continue;
    }
    if (result == BROTLI_DECODER_RESULT_SUCCESS) {
      decoded_buffer.resize(total_out);
      return fxcodec::ApplyPredictor(
          std::move(decoded_buffer), predictor_value, colors,
          bits_per_component, columns,
          static_cast<uint32_t>(src_span.subspan(available_in).size()));
    }
    return {DataVector<uint8_t>(), 0u};
  }
}

void BrotliDecoder::SetBrotliEnabled(bool enabled) {
  g_brotli_enabled = enabled;
}

bool BrotliDecoder::GetBrotliEnabled() {
  return g_brotli_enabled;
}
