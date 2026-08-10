// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PNG_SKIA_PNG_CONTEXT_H_
#define CORE_FXCODEC_PNG_SKIA_PNG_CONTEXT_H_

#include <memory>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/cfx_dibitmap.h"

class SkCodec;

namespace fxcodec {

class ProgressiveDecoderContextDelegate;

class SkiaPngContext final : public ProgressiveDecoderContext {
 public:
  // Caller needs to guarantee that `delegate` lives longer than
  // `SkiaPngContext`.
  explicit SkiaPngContext(ProgressiveDecoderContextDelegate* delegate);
  ~SkiaPngContext() override;

  // ProgressiveDecoderContext:
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);
  FXCODEC_STATUS ContinueDecode();

  bool ReadHeader(RetainPtr<CFX_CodecMemory> codec_memory);

 private:
  enum class State {
    // `decoder_` is null.
    //
    // This is the initial state.
    kNoDecoder,
    kGotDecoder,
    // `decoder_` is ready to accept incremental decoding inputs.
    //
    // This state is entered via:
    // * `StartDecode` - initial attempt to configure `decoder_`
    // * `kError` when `ContinueDecode` retries after receiving more input
    //   data from `delegate_->ReadMoreData(...)`
    //
    // From this state `ContinueDecode` transitions to:
    // * `kFinishedDecoding` - when all pixels have been decoded
    // * `kError` - when `decoder_` needs more input or encounters a fatal error
    kStartedDecode,
    // `decoder_` is null.
    //
    // This state is entered when `ContinueDecode` successfully decodes all
    // pixels.
    kFinishedDecoding,
    // `decoder_` is null.
    //
    // This state is entered when `ContinueDecode` encounters an error (either a
    // fatal decoding error, or a recoverable error when `decoder_` runs out of
    // input data).  From this state `ContinueDecode` attempts to read more data
    // from `delegate_->ReadMoreData(...)` and transitions to `kStartedDecode`
    // on success.
    kError,
  };

  bool ProcessData(RetainPtr<CFX_CodecMemory> codec_memory);

  State state_ = State::kNoDecoder;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_DIBitmap> bitmap_;
  std::unique_ptr<SkCodec> decoder_;

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
