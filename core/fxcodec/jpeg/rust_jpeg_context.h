// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JPEG_RUST_JPEG_CONTEXT_H_
#define CORE_FXCODEC_JPEG_RUST_JPEG_CONTEXT_H_

#include <memory>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CFX_DIBitmap;

namespace fxcodec {

class RustJpegContext final : public ProgressiveDecoderContext {
 public:
  explicit RustJpegContext(ProgressiveDecoderContextDelegate* delegate);
  ~RustJpegContext() override;

  bool ReadHeader();
  bool ProcessData();

  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }
  int num_components() const { return num_components_; }
  uint16_t x_density() const { return x_density_; }
  uint16_t y_density() const { return y_density_; }
  uint8_t density_unit() const { return density_unit_; }

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

  State state_ = State::kNoDecoder;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  int num_components_ = 0;
  ProgressiveDecoderContextDelegate::Format src_format_ =
      ProgressiveDecoderContextDelegate::Format::kInvalid;
  uint16_t x_density_ = 0;
  uint16_t y_density_ = 0;
  uint8_t density_unit_ = 0;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  RetainPtr<CFX_DIBitmap> bitmap_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_JPEG_RUST_JPEG_CONTEXT_H_
