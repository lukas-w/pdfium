// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_TIFF_LIBTIFF_TIFF_CONTEXT_H_
#define CORE_FXCODEC_TIFF_LIBTIFF_TIFF_CONTEXT_H_

#include <stdint.h>

#include <memory>

#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/retain_ptr.h"

#ifndef PDF_ENABLE_XFA
#error "XFA must be enabled"
#endif

struct tiff;
using TIFF = struct tiff;

class CFX_DIBitmap;
class IFX_SeekableReadStream;

namespace fxcodec {

class CFX_DIBAttribute;

struct TiffDeleter {
  void operator()(TIFF* context);
};

class LibtiffTiffContext final : public ProgressiveDecoderContext {
 public:
  LibtiffTiffContext();
  ~LibtiffTiffContext() override;

  bool InitDecoder(const RetainPtr<IFX_SeekableReadStream>& file_ptr);
  bool LoadFrameInfo(int32_t frame,
                     int32_t* width,
                     int32_t* height,
                     int32_t* comps,
                     int32_t* bpc,
                     CFX_DIBAttribute* pAttribute);
  // `bitmap` must be `FXDIB_Format::kBgra`.
  bool Decode(RetainPtr<CFX_DIBitmap> bitmap);

  RetainPtr<IFX_SeekableReadStream> io_in() const { return io_in_; }
  uint32_t offset() const { return offset_; }
  void set_offset(uint32_t offset) { offset_ = offset; }

 private:
  RetainPtr<IFX_SeekableReadStream> io_in_;
  uint32_t offset_ = 0;
  std::unique_ptr<TIFF, TiffDeleter> tif_ctx_;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_TIFF_LIBTIFF_TIFF_CONTEXT_H_
