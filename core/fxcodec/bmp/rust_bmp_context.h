// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_BMP_RUST_BMP_CONTEXT_H_
#define CORE_FXCODEC_BMP_RUST_BMP_CONTEXT_H_

#include <stdint.h>

#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {

class CFX_DIBAttribute;
class ProgressiveDecoderContextDelegate;

class RustBmpContext final : public ProgressiveDecoderContext {
 public:
  explicit RustBmpContext(ProgressiveDecoderContextDelegate* delegate);
  ~RustBmpContext() override;

  void SetCodecMemory(RetainPtr<CFX_CodecMemory> codec_memory);

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override;
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;
  ProgressiveDecoderContext::Status DecodeImage() override;

  ProgressiveDecoderContext::Status ReadHeader(
      int32_t* width,
      int32_t* height,
      int32_t* components,
      pdfium::span<const FX_ARGB>* palette,
      CFX_DIBAttribute* attribute);

  ProgressiveDecoderContext::Status StartDecode();
  ProgressiveDecoderContext::Status ContinueDecode();

 private:
  bool header_read_ = false;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  int32_t components_ = 0;
  int32_t dpi_x_ = 0;
  int32_t dpi_y_ = 0;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_BMP_RUST_BMP_CONTEXT_H_
