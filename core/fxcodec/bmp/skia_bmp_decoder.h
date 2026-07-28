// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_BMP_SKIA_BMP_DECODER_H_
#define CORE_FXCODEC_BMP_SKIA_BMP_DECODER_H_

#include <stdint.h>

#include <memory>

#include "core/fxcodec/bmp/bmp_decoder_delegate.h"
#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/fx_types.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

#if !defined(PDF_ENABLE_XFA_BMP)
#error "BMP must be enabled"
#endif

#if !defined(PDF_USE_SKIA)
#error "Skia must be enabled"
#endif

namespace fxcodec {

class CFX_DIBAttribute;

// BMP decoder that uses Skia's Rust BMP decoder to decode pixels.
class SkiaBmpDecoder {
 public:
  static std::unique_ptr<ProgressiveDecoderContext> StartDecode(
      BmpDecoderDelegate* delegate);
  static ProgressiveDecoderContext::Status ReadHeader(
      ProgressiveDecoderContext* context,
      int32_t* width,
      int32_t* height,
      bool* tb_flag,
      int32_t* components,
      pdfium::span<const FX_ARGB>* palette,
      CFX_DIBAttribute* attribute);
  static ProgressiveDecoderContext::Status DecodeImage(
      ProgressiveDecoderContext* context);

  SkiaBmpDecoder() = delete;
  SkiaBmpDecoder(const SkiaBmpDecoder&) = delete;
  SkiaBmpDecoder& operator=(const SkiaBmpDecoder&) = delete;
};

}  // namespace fxcodec

using BmpDecoder = fxcodec::SkiaBmpDecoder;

#endif  // CORE_FXCODEC_BMP_SKIA_BMP_DECODER_H_
