// Copyright 2025 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_H_
#define CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_H_

#include <stdint.h>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcrt/fx_types.h"
#include "core/fxcrt/retain_ptr.h"

#ifndef PDF_ENABLE_XFA
#error "XFA Only"
#endif

namespace fxcodec {

class ProgressiveDecoderContext {
 public:
  enum class Status : uint8_t {
    kError,
    kSuccess,
    kContinue,
  };

  virtual ~ProgressiveDecoderContext();

  virtual FX_FILESIZE GetAvailInput() const;
  virtual void Input(RetainPtr<CFX_CodecMemory> codec_memory);

  virtual Status DecodeImage();
};

}  // namespace fxcodec

using fxcodec::ProgressiveDecoderContext;

#endif  // CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_H_
