// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PNG_LIBPNG_PNG_CONTEXT_H_
#define CORE_FXCODEC_PNG_LIBPNG_PNG_CONTEXT_H_

#include <stdint.h>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/unowned_ptr_exclusion.h"
#include "core/fxge/dib/cfx_dibitmap.h"

struct png_info_def;
struct png_struct_def;

namespace fxcodec {

class ProgressiveDecoderContextDelegate;

class LibpngPngContext final : public ProgressiveDecoderContext {
 public:
  static constexpr size_t kPngErrorSize = 256;

  explicit LibpngPngContext(ProgressiveDecoderContextDelegate* delegate);
  ~LibpngPngContext() override;

  // ProgressiveDecoderContext:
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);
  FXCODEC_STATUS ContinueDecode();

  bool ReadHeader(RetainPtr<CFX_CodecMemory> codec_memory);

  UNOWNED_PTR_EXCLUSION png_struct_def* png_ = nullptr;
  UNOWNED_PTR_EXCLUSION png_info_def* info_ = nullptr;
  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_DIBitmap> bitmap_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  char last_error_[kPngErrorSize] = {};
  uint32_t height_ = 0;
  int number_of_passes_ = 0;
  bool finished_ = false;

 private:
  bool ProcessData(RetainPtr<CFX_CodecMemory> codec_memory);
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PNG_LIBPNG_PNG_CONTEXT_H_
