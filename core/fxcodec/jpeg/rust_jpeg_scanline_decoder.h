// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JPEG_RUST_JPEG_SCANLINE_DECODER_H_
#define CORE_FXCODEC_JPEG_RUST_JPEG_SCANLINE_DECODER_H_

#include <stdint.h>

#include <memory>
#include <optional>

#include "core/fxcodec/jpeg/jpegmodule.h"
#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/span.h"

namespace fxcodec {

class RustJpegScanlineDecoder final : public ScanlineDecoder {
 public:
  static std::unique_ptr<ScanlineDecoder> Create(
      pdfium::span<const uint8_t> src_span,
      uint32_t width,
      uint32_t height,
      int num_components,
      bool color_transform,
      uint32_t scale_denom);

  static std::optional<JpegModule::ImageInfo> LoadInfo(
      pdfium::span<const uint8_t> src_span);

  RustJpegScanlineDecoder();
  ~RustJpegScanlineDecoder() override;

  // ScanlineDecoder:
  [[nodiscard]] bool Rewind() override;
  pdfium::span<uint8_t> GetNextLine() override;
  uint32_t GetSrcOffset() override;

 private:
  bool CreateImpl(pdfium::span<const uint8_t> src_span,
                  uint32_t width,
                  uint32_t height,
                  int num_components,
                  bool color_transform,
                  uint32_t scale_denom);
  void CalcPitch();

  DataVector<uint8_t> decoded_image_;
  uint32_t next_row_ = 0;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_JPEG_RUST_JPEG_SCANLINE_DECODER_H_
