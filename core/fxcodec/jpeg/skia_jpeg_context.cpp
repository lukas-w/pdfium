// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/skia_jpeg_context.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/codec_memory_sk_stream.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/codec/SkJpegDecoder.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkStream.h"

namespace fxcodec {

SkiaJpegContext::SkiaJpegContext(ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {}

SkiaJpegContext::~SkiaJpegContext() = default;

bool SkiaJpegContext::ReadHeader(RetainPtr<CFX_CodecMemory> codec_memory) {
  if (state_ == State::kGotDecoder || state_ == State::kFinishedDecoding) {
    return true;
  }
  if (state_ == State::kError) {
    return false;
  }

  CHECK_EQ(state_, State::kNoDecoder);
  CHECK(!decoder_);

  if (!codec_memory_) {
    codec_memory_ = codec_memory;
  }
  CHECK_EQ(&*codec_memory_, &*codec_memory);

  auto stream = std::make_unique<CodecMemorySkStream>(std::move(codec_memory));
  SkCodec::Result result = SkCodec::kSuccess;
  decoder_ = SkJpegDecoder::Decode(std::move(stream), &result);
  switch (result) {
    case SkCodec::kSuccess: {
      SkImageInfo info = decoder_->getInfo();
      if (!delegate_->PrepareDirectOutput(
              info.width(), info.height(),
              ProgressiveDecoderContextDelegate::Format::kBgra)) {
        decoder_.reset();
        state_ = State::kError;
        return false;
      }
      state_ = State::kGotDecoder;
      return true;
    }
    case SkCodec::kIncompleteInput:
      codec_memory_->Seek(0);
      return true;
    default:
      decoder_.reset();
      state_ = State::kError;
      return false;
  }
}

bool SkiaJpegContext::ProcessData(RetainPtr<CFX_CodecMemory> codec_memory) {
  if (state_ == State::kNoDecoder) {
    if (!ReadHeader(std::move(codec_memory))) {
      return false;
    }
    if (state_ == State::kNoDecoder) {
      return true;
    }
  }

  if (state_ == State::kGotDecoder) {
    CHECK(bitmap_);
    SkImageInfo dst_info =
        decoder_->getInfo().makeColorType(kBGRA_8888_SkColorType);

    pdfium::span<uint8_t> dst_buffer = delegate_->AskImageBuf();
    FX_SAFE_SIZE_T row_bytes = dst_buffer.size();
    row_bytes /= dst_info.height();

    SkCodec::Result result = decoder_->getPixels(dst_info, dst_buffer.data(),
                                                 row_bytes.ValueOrDie());
    decoder_.reset();
    switch (result) {
      case SkCodec::kSuccess:
        state_ = State::kFinishedDecoding;
        return true;
      case SkCodec::kIncompleteInput:
        // SkJpegDecoder does not support incremental decoding. When
        // incomplete, retry the entire operation from the beginning.
        state_ = State::kNoDecoder;
        codec_memory_->Seek(0);
        return true;
      default:
        state_ = State::kError;
        return false;
    }
  }

  CHECK_EQ(state_, State::kFinishedDecoding);
  return true;
}

void SkiaJpegContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = std::move(codec_memory);
}

FXCODEC_STATUS SkiaJpegContext::StartDecode(RetainPtr<CFX_DIBitmap> bitmap) {
  bitmap_ = std::move(bitmap);
  CHECK_EQ(bitmap_->GetFormat(), FXDIB_Format::kBgra);
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeToBeContinued;
  if (!delegate_->ReadMoreData(0, &status)) {
    return status;
  }
  return FXCODEC_STATUS::kDecodeToBeContinued;
}

FXCODEC_STATUS SkiaJpegContext::ContinueDecode() {
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeFinished;
  while (state_ != State::kFinishedDecoding) {
    if (!ProcessData(codec_memory_)) {
      status = FXCODEC_STATUS::kError;
      break;
    }
    if (state_ == State::kFinishedDecoding) {
      break;
    }
    status = FXCODEC_STATUS::kError;
    if (!delegate_->ReadMoreData(std::nullopt, &status)) {
      break;
    }
  }
  bitmap_ = nullptr;
  return state_ == State::kFinishedDecoding ? FXCODEC_STATUS::kDecodeFinished
                                            : status;
}

}  // namespace fxcodec
