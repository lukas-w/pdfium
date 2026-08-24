// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PNG_RUST_PNG_CONTEXT_H_
#define CORE_FXCODEC_PNG_RUST_PNG_CONTEXT_H_

#include <stdint.h>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_DIBitmap;

namespace fxcodec {

class ProgressiveDecoderContextDelegate;

class RustPngContext final : public ProgressiveDecoderContext {
 public:
  explicit RustPngContext(ProgressiveDecoderContextDelegate* delegate);
  ~RustPngContext() override;

  bool ReadHeader();
  bool has_error() const { return state_ == State::kError; }

  // ProgressiveDecoderContext:
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);
  FXCODEC_STATUS ContinueDecode();

 private:
  enum class State {
    kNoDecoder,
    kGotHeader,
    kFinishedDecoding,
    kError,
  };

  bool ProcessData();

  State state_ = State::kNoDecoder;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  RetainPtr<CFX_DIBitmap> bitmap_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PNG_RUST_PNG_CONTEXT_H_
