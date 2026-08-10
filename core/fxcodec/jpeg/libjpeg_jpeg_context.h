// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JPEG_LIBJPEG_JPEG_CONTEXT_H_
#define CORE_FXCODEC_JPEG_LIBJPEG_JPEG_CONTEXT_H_

#include <vector>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/jpeg/jpeg_common.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

namespace fxcodec {

class CFX_DIBAttribute;
class ProgressiveDecoderContextDelegate;

class LibjpegJpegContext final : public ProgressiveDecoderContext {
 public:
  // Result codes for ReadHeader()/ReadScanline():
  static constexpr int kFatal = -1;
  static constexpr int kOk = 0;
  static constexpr int kError = 1;
  static constexpr int kNeedsMoreInput = 2;

  explicit LibjpegJpegContext(ProgressiveDecoderContextDelegate* delegate);
  ~LibjpegJpegContext() override;

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override;
  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override;

  FXCODEC_STATUS StartDecode();
  FXCODEC_STATUS ContinueDecode();

  int ReadHeader(int* width,
                 int* height,
                 int* nComps,
                 CFX_DIBAttribute* attribute);
  bool StartScanline();
  int ReadScanline(uint8_t* dest_buf);

  JpegCommon common_ = {};
  bool create_ok_ = false;

 private:
  void SyncCodecMemory();

  UnownedPtr<ProgressiveDecoderContextDelegate> const delegate_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  std::vector<uint8_t> line_buf_;
  int decode_row_ = 0;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_JPEG_LIBJPEG_JPEG_CONTEXT_H_
