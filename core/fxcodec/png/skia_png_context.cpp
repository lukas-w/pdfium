// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_context.h"

#include <utility>

#include "core/fxcodec/codec_memory_sk_stream.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkStream.h"

#ifdef PDF_ENABLE_RUST_PNG
#include "third_party/skia/include/codec/SkPngRustDecoder.h"
#else
#include "third_party/skia/include/codec/SkPngDecoder.h"
#endif

namespace fxcodec {

namespace {

constexpr double kPngGamma = 2.2;

sk_sp<SkColorSpace> GetTargetColorSpace() {
  const skcms_TransferFunction fn = {
      .g = static_cast<float>(1.0 / kPngGamma),
      .a = 1.0f,
  };
  return SkColorSpace::MakeRGB(fn, SkNamedGamut::kSRGB);
}

}  // namespace

SkiaPngContext::SkiaPngContext(ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {}

SkiaPngContext::~SkiaPngContext() = default;

bool SkiaPngContext::ReadHeader(RetainPtr<CFX_CodecMemory> codec_memory) {
  if (state_ == State::kGotDecoder || state_ == State::kStartedDecode ||
      state_ == State::kFinishedDecoding) {
    return true;
  }
  if (state_ == State::kError) {
    return false;
  }

  CHECK_EQ(state_, State::kNoDecoder);
  CHECK(!decoder_);

  // `ProgressiveDecoder` guarantees that all calls to
  // `ReadHeader`/`ProcessData` use the same `codec_memory`. Therefore
  // `SkiaPngContext` expects that `codec_memory` passed to
  // `CodecMemorySkStream` below remains the right one to use going forward.
  if (!codec_memory_) {
    codec_memory_ = codec_memory;
  }
  CHECK_EQ(&*codec_memory_, &*codec_memory);

  auto stream = std::make_unique<CodecMemorySkStream>(std::move(codec_memory));
  SkCodec::Result result = SkCodec::kSuccess;
#ifdef PDF_ENABLE_RUST_PNG
  decoder_ = SkPngRustDecoder::Decode(std::move(stream), &result);
#else
  decoder_ = SkPngDecoder::Decode(std::move(stream), &result);
#endif
  switch (result) {
    case SkCodec::kSuccess: {
      SkImageInfo info = decoder_->getInfo();
      // Notifies the delegate of image dimensions and metadata.
      if (!delegate_->PrepareDirectOutput(
              info.width(), info.height(),
              ProgressiveDecoderContextDelegate::Format::kArgb)) {
        decoder_.reset();
        state_ = State::kError;
        return false;
      }
      state_ = State::kGotDecoder;
      return true;
    }
    case SkCodec::kIncompleteInput:
      // Rewind to start from the beginning of input when retrying later.
      // This will also prompt `ProgressiveDecoder::ReadMoreData` to grow
      // the `codec_memory_` as needed.
      codec_memory_->Seek(0);
      return true;  // retry when called later with more data
    default:
      decoder_.reset();
      state_ = State::kError;
      return false;  // fatal error
  }
}

bool SkiaPngContext::ProcessData(RetainPtr<CFX_CodecMemory> codec_memory) {
  if (state_ == State::kNoDecoder) {
    if (!ReadHeader(std::move(codec_memory))) {
      return false;
    }
    if (state_ == State::kNoDecoder) {
      return true;  // Needs more input to finish reading header.
    }
  }

  if (state_ == State::kGotDecoder) {
    CHECK(bitmap_);
    SkImageInfo dst_info = decoder_->getInfo()
                               .makeColorSpace(GetTargetColorSpace())
                               .makeColorType(kBGRA_8888_SkColorType);

    pdfium::span<uint8_t> dst_buffer = delegate_->AskImageBuf();
    FX_SAFE_SIZE_T row_bytes = dst_buffer.size();
    row_bytes /= dst_info.height();

    SkCodec::Result result = decoder_->startIncrementalDecode(
        dst_info, dst_buffer.data(), row_bytes.ValueOrDie());
    switch (result) {
      case SkCodec::kSuccess:
        state_ = State::kStartedDecode;
        break;  // continue decoding
      case SkCodec::kIncompleteInput:
        return true;  // retry when called later with more data
      default:
        decoder_.reset();
        state_ = State::kError;
        return false;  // fatal error
    }
  }

  CHECK_EQ(state_, State::kStartedDecode);
  SkCodec::Result result = decoder_->incrementalDecode(nullptr);
  switch (result) {
    case SkCodec::kSuccess:
      decoder_.reset();
      state_ = State::kFinishedDecoding;
      return true;  // finished decoding
    case SkCodec::kIncompleteInput:
      return true;  // retry when called later with more data
    default:
      decoder_.reset();
      state_ = State::kError;
      return false;  // fatal error
  }
}

void SkiaPngContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = std::move(codec_memory);
}

FXCODEC_STATUS SkiaPngContext::StartDecode(RetainPtr<CFX_DIBitmap> bitmap) {
  bitmap_ = std::move(bitmap);
  CHECK_EQ(bitmap_->GetFormat(), FXDIB_Format::kBgra);
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeToBeContinued;
  if (!delegate_->ReadMoreData(0, &status)) {
    return status;
  }
  return FXCODEC_STATUS::kDecodeToBeContinued;
}

FXCODEC_STATUS SkiaPngContext::ContinueDecode() {
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
