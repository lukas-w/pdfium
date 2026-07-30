// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/bmp/skia_bmp_decoder.h"

#include "core/fxcodec/bmp/skia_bmp_context.h"

namespace fxcodec {

// static
ProgressiveDecoderContext::Status SkiaBmpDecoder::ReadHeader(
    ProgressiveDecoderContext* context,
    int32_t* width,
    int32_t* height,
    bool* tb_flag,
    int32_t* components,
    pdfium::span<const FX_ARGB>* palette,
    CFX_DIBAttribute* pAttribute) {
  auto* ctx = static_cast<SkiaBmpContext*>(context);
  return ctx->ReadHeader(width, height, tb_flag, components, palette,
                         pAttribute);
}

}  // namespace fxcodec
