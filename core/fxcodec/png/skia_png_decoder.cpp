// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/skia_png_decoder.h"

#include <memory>
#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcodec/png/skia_png_context.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"

namespace fxcodec {

// static
std::unique_ptr<ProgressiveDecoderContext> SkiaPngDecoder::StartDecode(
    PngDecoderDelegate* delegate) {
  return std::make_unique<SkiaPngContext>(delegate);
}

// static
bool SkiaPngDecoder::ContinueDecode(ProgressiveDecoderContext* context,
                                    RetainPtr<CFX_CodecMemory> codec_memory) {
  auto* ctx = static_cast<SkiaPngContext*>(context);
  return ctx->ContinueDecode(std::move(codec_memory));
}

}  // namespace fxcodec
