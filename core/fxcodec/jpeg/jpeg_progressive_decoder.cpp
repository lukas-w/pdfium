// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jpeg/jpeg_progressive_decoder.h"

#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/jpeg/jpeg_common.h"
#include "core/fxcodec/jpeg/libjpeg_jpeg_context.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/fx_dib.h"

static void JpegLoadAttribute(const jpeg_decompress_struct& info,
                              CFX_DIBAttribute* pAttribute) {
  pAttribute->x_dpi_ = info.X_density;
  pAttribute->y_dpi_ = info.Y_density;
  pAttribute->dpi_unit_ =
      static_cast<CFX_DIBAttribute::ResUnit>(info.density_unit);
}

namespace fxcodec {

// static
int JpegProgressiveDecoder::ReadHeader(ProgressiveDecoderContext* context,
                                       int* width,
                                       int* height,
                                       int* nComps,
                                       CFX_DIBAttribute* pAttribute) {
  DCHECK(pAttribute);

  auto* ctx = static_cast<LibjpegJpegContext*>(context);
  int ret = jpeg_common_read_header(&ctx->common_, TRUE);
  if (ret == -1) {
    return kFatal;
  }
  if (ret == JPEG_SUSPENDED) {
    return kNeedsMoreInput;
  }
  if (ret != JPEG_HEADER_OK) {
    return kError;
  }
  *width = ctx->common_.cinfo.image_width;
  *height = ctx->common_.cinfo.image_height;
  *nComps = ctx->common_.cinfo.num_components;
  JpegLoadAttribute(ctx->common_.cinfo, pAttribute);
  return kOk;
}

// static
bool JpegProgressiveDecoder::StartScanline(ProgressiveDecoderContext* context) {
  auto* ctx = static_cast<LibjpegJpegContext*>(context);
  ctx->common_.cinfo.scale_denom = 1;
  return !!jpeg_common_start_decompress(&ctx->common_);
}

// static
int JpegProgressiveDecoder::ReadScanline(ProgressiveDecoderContext* context,
                                         unsigned char* dest_buf) {
  auto* ctx = static_cast<LibjpegJpegContext*>(context);
  int nlines = jpeg_common_read_scanlines(&ctx->common_, &dest_buf, 1);
  if (nlines == -1) {
    return kFatal;
  }
  return nlines == 1 ? kOk : kError;
}

}  // namespace fxcodec
