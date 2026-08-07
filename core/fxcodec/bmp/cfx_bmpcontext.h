// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_BMP_CFX_BMPCONTEXT_H_
#define CORE_FXCODEC_BMP_CFX_BMPCONTEXT_H_

#include "core/fxcodec/bmp/cfx_bmpdecompressor.h"
#include "core/fxcodec/bmp/fx_bmp.h"
#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

namespace fxcodec {

class CFX_DIBAttribute;
class ProgressiveDecoderContextDelegate;

class CFX_BmpContext final : public ProgressiveDecoderContext {
 public:
  explicit CFX_BmpContext(ProgressiveDecoderContextDelegate* pDelegate);
  ~CFX_BmpContext() override;

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override;
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;
  Status DecodeImage() override;

  Status ReadHeader(int32_t* width,
                    int32_t* height,
                    int32_t* components,
                    pdfium::span<const FX_ARGB>* palette,
                    CFX_DIBAttribute* attribute);

  CFX_BmpDecompressor bmp_;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_BMP_CFX_BMPCONTEXT_H_
