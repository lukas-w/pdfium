// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JPEG_LIBJPEG_JPEG_CONTEXT_H_
#define CORE_FXCODEC_JPEG_LIBJPEG_JPEG_CONTEXT_H_

#include "core/fxcodec/jpeg/jpeg_common.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"

namespace fxcodec {

class CFX_DIBAttribute;

class LibjpegJpegContext final : public ProgressiveDecoderContext {
 public:
  // Result codes for ReadHeader()/ReadScanline():
  static constexpr int kFatal = -1;
  static constexpr int kOk = 0;
  static constexpr int kError = 1;
  static constexpr int kNeedsMoreInput = 2;

  LibjpegJpegContext();
  ~LibjpegJpegContext() override;

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override;
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  int ReadHeader(int* width,
                 int* height,
                 int* nComps,
                 CFX_DIBAttribute* pAttribute);
  bool StartScanline();
  int ReadScanline(uint8_t* dest_buf);

  JpegCommon common_ = {};
  bool create_ok_ = false;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_JPEG_LIBJPEG_JPEG_CONTEXT_H_
