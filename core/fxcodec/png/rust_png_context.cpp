// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/rust_png_context.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/rust_png_ffi.rs.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"

namespace fxcodec {

RustPngContext::RustPngContext(ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {}

RustPngContext::~RustPngContext() = default;

bool RustPngContext::ReadHeader() {
  if (state_ == State::kGotHeader || state_ == State::kFinishedDecoding) {
    return true;
  }
  if (state_ == State::kError || !codec_memory_) {
    return false;
  }
  CHECK_EQ(state_, State::kNoDecoder);

  rust_png::PngHeaderInfo header_info{};
  pdfium::span<const uint8_t> src_span = codec_memory_->GetBufferSpan();
  rust::Slice<const uint8_t> src_slice(src_span);
  if (!rust_png::read_png_info(src_slice, header_info)) {
    return false;
  }

  width_ = header_info.width;
  height_ = header_info.height;
  if (!delegate_->PrepareDirectOutput(
          width_, height_, ProgressiveDecoderContextDelegate::Format::kArgb)) {
    state_ = State::kError;
    return false;
  }
  state_ = State::kGotHeader;
  return true;
}

bool RustPngContext::ProcessData() {
  if (state_ == State::kNoDecoder) {
    if (!ReadHeader()) {
      return false;
    }
  }

  if (state_ == State::kGotHeader) {
    pdfium::span<const uint8_t> src_span = codec_memory_->GetBufferSpan();
    rust::Slice<const uint8_t> src_slice(src_span);
    pdfium::span<uint8_t> dst_buffer = delegate_->AskImageBuf();
    uint32_t pitch = bitmap_->GetPitch();
    rust::Slice<uint8_t> dst_slice(dst_buffer);
    if (!rust_png::decode_png_to_buf(src_slice, dst_slice, pitch)) {
      return false;
    }
    state_ = State::kFinishedDecoding;
    return true;
  }

  CHECK_EQ(state_, State::kFinishedDecoding);
  return true;
}

void RustPngContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = std::move(codec_memory);
}

FXCODEC_STATUS RustPngContext::StartDecode(RetainPtr<CFX_DIBitmap> bitmap) {
  bitmap_ = std::move(bitmap);
  CHECK_EQ(bitmap_->GetFormat(), FXDIB_Format::kBgra);
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeToBeContinued;
  if (!delegate_->ReadMoreData(0, &status)) {
    return status;
  }
  return FXCODEC_STATUS::kDecodeToBeContinued;
}

FXCODEC_STATUS RustPngContext::ContinueDecode() {
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeFinished;
  while (state_ != State::kFinishedDecoding) {
    if (!ProcessData()) {
      status = FXCODEC_STATUS::kError;
      if (!delegate_->ReadMoreData(std::nullopt, &status)) {
        if (status != FXCODEC_STATUS::kDecodeToBeContinued) {
          state_ = State::kError;
        }
        break;
      }
      continue;
    }
    if (state_ == State::kFinishedDecoding) {
      break;
    }
  }
  if (status != FXCODEC_STATUS::kDecodeToBeContinued) {
    bitmap_ = nullptr;
  }
  return state_ == State::kFinishedDecoding ? FXCODEC_STATUS::kDecodeFinished
                                            : status;
}

}  // namespace fxcodec
