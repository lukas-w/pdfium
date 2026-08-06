// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_
#define CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_

#include <stdint.h>

namespace fxcodec {

class ProgressiveDecoderContextDelegate {
 public:
  // Do not use `FXDIB_Format` here since these decoders can return pixel
  // formats not supported by it.
  enum class Format : uint16_t {
    kInvalid = 0,
    k8bppGray = 0x108,
    k8bppRgb = 0x008,
    kRgb = 0x018,
    kRgb32 = 0x020,
    kArgb = 0x220,
    kCmyk = 0x120,
  };

  virtual ~ProgressiveDecoderContextDelegate() = default;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_
