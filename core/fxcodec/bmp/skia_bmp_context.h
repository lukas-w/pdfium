// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_BMP_SKIA_BMP_CONTEXT_H_
#define CORE_FXCODEC_BMP_SKIA_BMP_CONTEXT_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/fx_dib.h"

class SkCodec;

namespace fxcodec {

class BmpDecoderDelegate;
class CFX_DIBAttribute;

class SkiaBmpContext final : public ProgressiveDecoderContext {
 public:
  explicit SkiaBmpContext(BmpDecoderDelegate* delegate);
  ~SkiaBmpContext() override;

  void SetCodecMemory(RetainPtr<CFX_CodecMemory> codec_memory);

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override;
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  ProgressiveDecoderContext::Status ReadHeader(
      int32_t* width,
      int32_t* height,
      bool* tb_flag,
      int32_t* components,
      pdfium::span<const FX_ARGB>* palette,
      CFX_DIBAttribute* pAttribute);

  ProgressiveDecoderContext::Status DecodeImage(size_t frame_index) override;

 private:
  bool ValidatePaletteIndices();
  ProgressiveDecoderContext::Status StartIncrementalDecode();
  void Forward3ComponentRow(uint32_t dest_row,
                            pdfium::span<const uint8_t> src_row,
                            pdfium::span<uint8_t> row_buffer,
                            int width);

  UnownedPtr<BmpDecoderDelegate> const delegate_;
  std::unique_ptr<SkCodec> decoder_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  std::vector<uint8_t> decoded_image_buf_;
  int rows_forwarded_ = 0;
  int components_ = 3;
  uint32_t bpp_ = 24;
  uint32_t bi_size_ = 40;
  uint32_t bi_compression_ = 0;
  uint32_t bi_clr_used_ = 0;
  bool top_down_ = false;
  bool header_read_ = false;
  bool decode_started_ = false;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_BMP_SKIA_BMP_CONTEXT_H_
