// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_context.h"

#include <utility>

#include "core/fxcodec/codec_memory_sk_stream.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span.h"
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

sk_sp<SkColorSpace> GetTargetColorSpace(double gamma) {
  const skcms_TransferFunction fn = {
      .g = static_cast<float>(1.0 / gamma),
      .a = 1.0f,
  };
  return SkColorSpace::MakeRGB(fn, SkNamedGamut::kSRGB);
}

}  // namespace

SkiaPngContext::SkiaPngContext(PngDecoderDelegate* delegate)
    : delegate_(delegate) {}

SkiaPngContext::~SkiaPngContext() = default;

bool SkiaPngContext::ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory) {
  // `ProgressiveDecoder` guarantees that all calls to `ContinueDecode` use the
  // same `codec_memory`.  Therefore `SkiaPngContext` expects that
  // `codec_memory` passed to `CodecMemorySkStream` below remains the right one
  // to use going forward.
  if (!codec_memory_) {
    codec_memory_ = codec_memory;
  }
  CHECK_EQ(&*codec_memory_, &*codec_memory);

  switch (state_) {
    case State::kNoDecoder: {
      CHECK(!decoder_);
      auto stream =
          std::make_unique<CodecMemorySkStream>(std::move(codec_memory));
      SkCodec::Result result = SkCodec::kSuccess;
#ifdef PDF_ENABLE_RUST_PNG
      decoder_ = SkPngRustDecoder::Decode(std::move(stream), &result);
#else
      decoder_ = SkPngDecoder::Decode(std::move(stream), &result);
#endif
      switch (result) {
        case SkCodec::kSuccess: {
          SkImageInfo info = decoder_->getInfo();
          if (!delegate_->PngReadHeader(info.width(), info.height(),
                                        &target_gamma_)) {
            decoder_.reset();
            state_ = State::kError;
            return false;
          }
          state_ = State::kGotDecoder;
          break;  // continue decoding
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
      break;
    }
    case State::kGotDecoder:
    case State::kStartedDecode:
      break;
    case State::kFinishedDecoding:
    case State::kError:
      NOTREACHED();
  }

  if (state_ == State::kGotDecoder) {
    SkImageInfo dst_info =
        decoder_->getInfo()
            .makeColorSpace(GetTargetColorSpace(target_gamma_))
            .makeColorType(kBGRA_8888_SkColorType);

    pdfium::span<uint8_t> dst_buffer = delegate_->PngAskImageBuf();
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
      delegate_->PngFinishedDecoding();
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

}  // namespace fxcodec
