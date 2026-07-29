// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PNG_SKIA_PNG_CONTEXT_H_
#define CORE_FXCODEC_PNG_SKIA_PNG_CONTEXT_H_

#include <memory>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class SkCodec;

namespace fxcodec {

class SkiaPngContext final : public ProgressiveDecoderContext {
 public:
  // Caller needs to guarantee that `delegate` lives longer than
  // `SkiaPngContext`.
  explicit SkiaPngContext(PngDecoderDelegate* delegate);
  ~SkiaPngContext() override;

  // Starts or resumes decoding `codec_memory`.
  //
  // Returns `false` upon failure.  Returns `true` when either 1) the whole
  // image has been successfully decoded or 2) the image has been partially
  // decoded but decoding should be continued/retried when more input data
  // is available.
  //
  // Communicates image metadata (once read/available) via
  // `PngDecoderDelegate::PngReadHeader`.  Writes decoded BGRA pixels to the
  // buffer provided via `PngDecoderDelegate::PngAskImageBuf`.
  bool ContinueDecode(RetainPtr<CFX_CodecMemory> codec_memory);

 private:
  enum class State {
    // `decoder_` is null.
    //
    // This is the initial state.
    kNoDecoder,

    // `decoder_` is not null and `delegate_->PngReadHeader` succeeded
    // and provided `target_gamma_`.
    //
    // `startIncrementalDecode` didn't run, or returned `kIncompleteInput`.
    kGotDecoder,

    // `decoder_` is not null, got `target_gamma_`, and `startIncrementalDecode`
    // has already suceeded.
    //
    // `incrementalDecode` didn't run yet, or returned `kIncompleteInput`.
    kStartedDecode,

    // `decoder_` is null.  All pixels have been decoded to
    // `delegate_->PngAskImageBuf`.
    //
    // This is a terminal state.
    kFinishedDecoding,

    // `decoder_` is null.  A non-recoverable (i.e. non-`kIncompleteInput`-kind)
    // error has been encountered.
    //
    // This is a terminal state.
    kError,
  };
  State state_ = State::kNoDecoder;

  UnownedPtr<PngDecoderDelegate> const delegate_;
  std::unique_ptr<SkCodec> decoder_;
  double target_gamma_ = 0.0;

  // `CFX_CodecMemory` received in `ContinueDecode` may get wrapped in
  // `CodecMemorySkStream` and become transitively owned by `decoder_`.  This
  // class retains `codec_memory_` to `CHECK` that all calls to `ContinueDecode`
  // use the same `CFX_CodecMemory` - this helps to ensure that `decoder_` and
  // `CodecMemorySkStream` won't accidentally use stale input data if a future,
  // hypothetical refactoring of `ProgressiveDecoder` changes how it manages
  // `CFX_CodecMemory`.
  RetainPtr<CFX_CodecMemory> codec_memory_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PNG_SKIA_PNG_CONTEXT_H_
