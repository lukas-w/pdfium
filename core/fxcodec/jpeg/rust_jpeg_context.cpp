// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/rust_jpeg_context.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/jpeg/rust_jpeg_ffi.rs.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"

namespace fxcodec {

RustJpegContext::RustJpegContext(ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {}

RustJpegContext::~RustJpegContext() = default;

bool RustJpegContext::ReadHeader() {
  if (state_ == State::kGotHeader || state_ == State::kFinishedDecoding) {
    return true;
  }
  if (state_ == State::kError || !codec_memory_) {
    return false;
  }

  CHECK_EQ(state_, State::kNoDecoder);

  pdfium::span<const uint8_t> src_span = codec_memory_->GetBufferSpan();
  rust_jpeg::JpegHeaderInfo header_info{};
  rust::Slice<const uint8_t> src_slice(src_span);
  if (!rust_jpeg::read_jpeg_info(src_slice, header_info)) {
    // More data may be needed to parse the header during progressive decoding,
    // so do not transition to `State::kError` here.
    return false;
  }

  switch (header_info.num_components) {
    case 1:
      src_format_ = ProgressiveDecoderContextDelegate::Format::k8bppGray;
      break;
    case 3:
      src_format_ = ProgressiveDecoderContextDelegate::Format::kRgb;
      break;
    case 4:
      src_format_ = ProgressiveDecoderContextDelegate::Format::kCmyk;
      break;
    default:
      state_ = State::kError;
      return false;
  }

  width_ = header_info.width;
  height_ = header_info.height;
  num_components_ = header_info.num_components;
  x_density_ = header_info.x_density;
  y_density_ = header_info.y_density;
  density_unit_ = header_info.density_unit;

  state_ = State::kGotHeader;
  return true;
}

bool RustJpegContext::ProcessData() {
  if (state_ == State::kFinishedDecoding) {
    return true;
  }
  if (state_ == State::kError) {
    return false;
  }

  if (state_ == State::kNoDecoder && !ReadHeader()) {
    return false;
  }

  CHECK_EQ(state_, State::kGotHeader);

  if (!delegate_->PrepareScanlineResampling(width_, height_, src_format_)) {
    state_ = State::kError;
    return false;
  }

  pdfium::span<const uint8_t> src_span = codec_memory_->GetBufferSpan();
  rust::Slice<const uint8_t> src_slice(src_span);

  FX_SAFE_SIZE_T row_bytes = width_;
  row_bytes *= num_components_;
  row_bytes += 3;
  row_bytes /= 4;
  row_bytes *= 4;
  if (!row_bytes.IsValid()) {
    state_ = State::kError;
    return false;
  }

  FX_SAFE_SIZE_T total_bytes = row_bytes;
  total_bytes *= height_;
  if (!total_bytes.IsValid()) {
    state_ = State::kError;
    return false;
  }

  DataVector<uint8_t> decoded_buf(total_bytes.ValueOrDie());
  rust::Slice<uint8_t> out_slice(decoded_buf);
  if (!rust_jpeg::decode_jpeg_to_buf(src_slice, out_slice,
                                     row_bytes.ValueOrDie(),
                                     /*scale_denom=*/1)) {
    state_ = State::kError;
    return false;
  }

  size_t stride = row_bytes.ValueOrDie();
  for (uint32_t y = 0; y < height_; ++y) {
    pdfium::span<const uint8_t> scanline =
        pdfium::span(decoded_buf).subspan(y * stride, stride);
    delegate_->ResampleScanline(y, scanline);
  }

  state_ = State::kFinishedDecoding;
  return true;
}

void RustJpegContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = std::move(codec_memory);
}

FXCODEC_STATUS RustJpegContext::StartDecode(RetainPtr<CFX_DIBitmap> bitmap) {
  bitmap_ = std::move(bitmap);
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeToBeContinued;
  if (!delegate_->ReadMoreData(0, &status)) {
    return status;
  }
  return FXCODEC_STATUS::kDecodeToBeContinued;
}

FXCODEC_STATUS RustJpegContext::ContinueDecode() {
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeFinished;
  while (delegate_->ReadMoreData(std::nullopt, &status)) {
  }
  if (status == FXCODEC_STATUS::kError || !ProcessData()) {
    state_ = State::kError;
    status = FXCODEC_STATUS::kError;
  }
  bitmap_ = nullptr;
  return state_ == State::kFinishedDecoding ? FXCODEC_STATUS::kDecodeFinished
                                            : status;
}

}  // namespace fxcodec
