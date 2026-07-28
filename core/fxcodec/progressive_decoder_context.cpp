// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/progressive_decoder_context.h"

namespace fxcodec {

ProgressiveDecoderContext::~ProgressiveDecoderContext() = default;

FX_FILESIZE ProgressiveDecoderContext::GetAvailInput() const {
  return 0;
}

void ProgressiveDecoderContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
}

ProgressiveDecoderContext::Status ProgressiveDecoderContext::DecodeImage(
    size_t frame_index) {
  return Status::kError;
}

}  // namespace fxcodec
