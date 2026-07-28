// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/bmp/cfx_bmpcontext.h"

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"

namespace fxcodec {

CFX_BmpContext::CFX_BmpContext(BmpDecoderDelegate* pDelegate)
    : bmp_(this), delegate_(pDelegate) {}

CFX_BmpContext::~CFX_BmpContext() = default;

FX_FILESIZE CFX_BmpContext::GetAvailInput() const {
  return bmp_.GetAvailInput();
}

void CFX_BmpContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  bmp_.SetInputBuffer(std::move(codec_memory));
}

}  // namespace fxcodec
