// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/bmp/cfx_bmpcontext.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcrt/check.h"

namespace fxcodec {

CFX_BmpContext::CFX_BmpContext(ProgressiveDecoderContextDelegate* pDelegate)
    : bmp_(this), delegate_(pDelegate) {}

CFX_BmpContext::~CFX_BmpContext() = default;

FX_FILESIZE CFX_BmpContext::GetAvailInput() const {
  return bmp_.GetAvailInput();
}

void CFX_BmpContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  bmp_.SetInputBuffer(std::move(codec_memory));
}

ProgressiveDecoderContext::Status CFX_BmpContext::DecodeImage(
    size_t frame_index) {
  return bmp_.DecodeImage();
}

ProgressiveDecoderContext::Status CFX_BmpContext::ReadHeader(
    int32_t* width,
    int32_t* height,
    int32_t* components,
    pdfium::span<const FX_ARGB>* palette,
    CFX_DIBAttribute* attribute) {
  DCHECK(attribute);

  ProgressiveDecoderContext::Status status = bmp_.ReadHeader();
  if (status != ProgressiveDecoderContext::Status::kSuccess) {
    return status;
  }

  *width = bmp_.width();
  *height = bmp_.height();
  *components = bmp_.components();
  *palette = bmp_.palette();
  attribute->dpi_unit_ = CFX_DIBAttribute::kResUnitMeter;
  attribute->x_dpi_ = bmp_.dpi_x();
  attribute->y_dpi_ = bmp_.dpi_y();
  return ProgressiveDecoderContext::Status::kSuccess;
}

}  // namespace fxcodec
