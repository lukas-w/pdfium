// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/gif/gif_decoder.h"

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/gif/cfx_gifcontext.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {

// static
std::unique_ptr<ProgressiveDecoderContext> GifDecoder::StartDecode(
    Delegate* pDelegate) {
  return std::make_unique<CFX_GifContext>(pDelegate);
}

// static
ProgressiveDecoderContext::Status GifDecoder::ReadHeader(
    ProgressiveDecoderContext* context,
    int* width,
    int* height,
    pdfium::span<CFX_GifPalette>* palette,
    int* bg_index) {
  auto* ctx = static_cast<CFX_GifContext*>(context);
  ProgressiveDecoderContext::Status ret = ctx->ReadHeader();
  if (ret != ProgressiveDecoderContext::Status::kSuccess) {
    return ret;
  }

  *width = ctx->width_;
  *height = ctx->height_;
  *palette = ctx->global_palette_;
  *bg_index = ctx->bc_index_;
  return ProgressiveDecoderContext::Status::kSuccess;
}

// static
std::pair<ProgressiveDecoderContext::Status, size_t> GifDecoder::LoadFrameInfo(
    ProgressiveDecoderContext* context) {
  auto* ctx = static_cast<CFX_GifContext*>(context);
  ProgressiveDecoderContext::Status ret = ctx->GetFrame();
  if (ret != ProgressiveDecoderContext::Status::kSuccess) {
    return {ret, 0};
  }
  return {ProgressiveDecoderContext::Status::kSuccess, ctx->GetFrameNum()};
}



}  // namespace fxcodec
