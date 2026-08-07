// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_
#define CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_

#include <stdint.h>

#include <optional>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {

class ProgressiveDecoderContextDelegate {
 public:
  // Do not use `FXDIB_Format` here since these decoders can return pixel
  // formats not supported by it.
  enum class Format : uint16_t {
    kInvalid = 0,
    k8bppGray = 0x108,
    k8bppRgb = 0x008,
    kRgb = 0x018,
    kRgb32 = 0x020,
    kArgb = 0x220,
    kCmyk = 0x120,
  };

  virtual ~ProgressiveDecoderContextDelegate() = default;

  // I/O: Refills input buffer from the file stream when decoder runs out of
  // data. If rcd_pos has a value, sets offset_ to rcd_pos before refilling.
  virtual bool ReadMoreData(std::optional<uint32_t> rcd_pos,
                            FXCODEC_STATUS* err_status) = 0;

  // Returns the current reading offset in the underlying input stream.
  virtual uint32_t GetCurrentInputPosition() const = 0;

  // Codec decoders use one of two mutually exclusive output paths:
  //
  // A. Scanline Resampling (Methods 1 and 2; e.g., BMP, GIF, JPEG):
  //    Used by streaming line-by-line decoders or whenever scaling,
  //    vertical orientation flipping, or color conversion is required.
  //    Accepts arbitrary source pixel formats (k8bppGray, kRgb, kCmyk,
  //    etc.) and resamples each scanline into the destination bitmap.
  //
  // B. Direct In-Place Output (Methods 3 and 4; e.g., PNG):
  //    Used by decoders that decompress directly into destination
  //    memory without intermediate buffers. Strictly requires 1:1
  //    scaling and an ARGB/BGRA destination format so decompressed
  //    bytes can be written safely in-place.

  // 1. Scaling: Prepares resampler weights and scanline buffer. Optionally sets
  // the color palette and fills the destination bitmap with a background color.
  // Returns true to proceed with decoding, or false to abort (e.g. during image
  // type detection).
  virtual bool PrepareScanlineResampling(
      int src_width,
      int src_height,
      Format src_format,
      pdfium::span<const FX_ARGB> palette = {},
      std::optional<FX_ARGB> fill_argb = std::nullopt) = 0;

  // 2. Scaling: Resamples a single decoded scanline into the device bitmap.
  virtual void ResampleScanline(int line,
                                pdfium::span<const uint8_t> src_span) = 0;

  // 3. Direct Output: Returns a writable span for a scanline in the device
  // bitmap.
  virtual pdfium::span<uint8_t> AskScanlineBuf(int line) = 0;

  // 4. Direct Output: Returns a writable span for the entire device bitmap.
  virtual pdfium::span<uint8_t> AskImageBuf() = 0;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_
