// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JPEG_SKIA_JPEG_CONTEXT_H_
#define CORE_FXCODEC_JPEG_SKIA_JPEG_CONTEXT_H_

#include <memory>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_CodecMemory;
class CFX_DIBitmap;
class SkCodec;

namespace fxcodec {

class ProgressiveDecoderContextDelegate;

class SkiaJpegContext final : public ProgressiveDecoderContext {
 public:
  // Caller needs to guarantee that `delegate` lives longer than
  // `SkiaJpegContext`.
  explicit SkiaJpegContext(ProgressiveDecoderContextDelegate* delegate);
  ~SkiaJpegContext() override;

  // ProgressiveDecoderContext:
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);
  FXCODEC_STATUS ContinueDecode();

  bool ReadHeader(RetainPtr<CFX_CodecMemory> codec_memory);

 private:
  enum class State {
    kNoDecoder,
    kGotDecoder,
    kFinishedDecoding,
    kError,
  };

  bool ProcessData(RetainPtr<CFX_CodecMemory> codec_memory);

  State state_ = State::kNoDecoder;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_DIBitmap> bitmap_;
  std::unique_ptr<SkCodec> decoder_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_JPEG_SKIA_JPEG_CONTEXT_H_
