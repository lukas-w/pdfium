// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_BMP_BMP_DECODER_H_
#define CORE_FXCODEC_BMP_BMP_DECODER_H_

#include <stdint.h>

#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

#ifndef PDF_ENABLE_XFA_BMP
#error "BMP must be enabled"
#endif

namespace fxcodec {

class CFX_DIBAttribute;

class BmpDecoder {
 public:
  static ProgressiveDecoderContext::Status ReadHeader(
      ProgressiveDecoderContext* context,
      int32_t* width,
      int32_t* height,
      bool* tb_flag,
      int32_t* components,
      pdfium::span<const FX_ARGB>* palette,
      CFX_DIBAttribute* pAttribute);

  // Only `static` methods.
  BmpDecoder() = delete;
  BmpDecoder(const BmpDecoder&) = delete;
  BmpDecoder& operator=(const BmpDecoder&) = delete;
};

}  // namespace fxcodec

using BmpDecoder = fxcodec::BmpDecoder;

#endif  // CORE_FXCODEC_BMP_BMP_DECODER_H_
