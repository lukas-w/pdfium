// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PNG_CPNGCONTEXT_H_
#define CORE_FXCODEC_PNG_CPNGCONTEXT_H_

#include <stdint.h>

#include "core/fxcodec/png/png_decoder_delegate.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/unowned_ptr_exclusion.h"

struct png_info_def;
struct png_struct_def;

namespace fxcodec {

class CPngContext final : public ProgressiveDecoderContext {
 public:
  static constexpr size_t kPngErrorSize = 256;

  explicit CPngContext(PngDecoderDelegate* pDelegate);
  ~CPngContext() override;

  UNOWNED_PTR_EXCLUSION png_struct_def* png_ = nullptr;
  UNOWNED_PTR_EXCLUSION png_info_def* info_ = nullptr;
  UnownedPtr<PngDecoderDelegate> const delegate_;
  char last_error_[kPngErrorSize] = {};
  uint32_t height_ = 0;
  int number_of_passes_ = 0;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PNG_CPNGCONTEXT_H_
