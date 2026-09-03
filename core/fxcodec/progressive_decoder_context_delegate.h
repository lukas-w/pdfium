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
  // formats not supported by it. The LSB is the bits-per-pixel, and the
  // MSB just needs ensure these are distinct.
  enum class Format : uint16_t {
    kInvalid = 0,
    k8bppGray = 0x108,
    k8bppRgb = 0x008,
    kBgr = 0x018,
    kBgrx = 0x020,
    kBgra = 0x220,
    kCmyk = 0x120,
  };

  virtual ~ProgressiveDecoderContextDelegate() = default;

  // I/O: Refills input buffer from the file stream when decoder runs out of
  // data. If rcd_pos has a value, sets offset_ to rcd_pos before refilling.
  virtual bool ReadMoreData(std::optional<uint32_t> rcd_pos,
                            FXCODEC_STATUS* err_status) = 0;

  // Returns the current reading offset in the underlying input stream.
  virtual uint32_t GetCurrentInputPosition() const = 0;

  // Scanline Resampling Output:
  // Prepares resampling weights and scanline buffer (for codecs that perform
  // resampling), and optionally sets the color palette and fills the
  // destination bitmap with a background color.
  // Returns true to proceed with decoding, or false to abort (e.g. when
  // no destination bitmap is available).
  virtual bool PrepareScanlineResampling(
      int src_width,
      int src_height,
      Format src_format,
      pdfium::span<const FX_ARGB> palette = {},
      std::optional<FX_ARGB> fill_argb = std::nullopt) = 0;

  // Resamples a single decoded scanline into the device bitmap (e.g., BMP,
  // GIF, JPEG).
  virtual void ResampleScanline(int line,
                                pdfium::span<const uint8_t> src_span) = 0;

  // Direct In-Place Output:
  // Notifies the delegate of image dimensions and metadata for codecs that
  // write directly into the device bitmap without scanline resampling (e.g.,
  // PNG).
  virtual bool PrepareDirectOutput(int src_width,
                                   int src_height,
                                   Format src_format) = 0;

  // Returns a writable span for a scanline or the entire image in the device
  // bitmap without intermediate scanline resampling (e.g., PNG).
  virtual pdfium::span<uint8_t> AskScanlineBuf(int line) = 0;
  virtual pdfium::span<uint8_t> AskImageBuf() = 0;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_PROGRESSIVE_DECODER_CONTEXT_DELEGATE_H_
