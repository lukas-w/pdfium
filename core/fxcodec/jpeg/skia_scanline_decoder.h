// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_JPEG_SKIA_SCANLINE_DECODER_H_
#define CORE_FXCODEC_JPEG_SKIA_SCANLINE_DECODER_H_

#include <stdint.h>

#include <memory>
#include <optional>

#include "core/fxcodec/jpeg/jpegmodule.h"
#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/span.h"

class SkCodec;

namespace fxcodec {

class SkiaScanlineDecoder final : public ScanlineDecoder {
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

  SkiaScanlineDecoder();
  ~SkiaScanlineDecoder() override;

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

  std::unique_ptr<SkCodec> decoder_;
  DataVector<uint8_t> scanline_buf_;
  DataVector<uint8_t> row_decode_buf_;
  uint32_t scale_denom_ = 1;
};

}  // namespace fxcodec

#endif  // CORE_FXCODEC_JPEG_SKIA_SCANLINE_DECODER_H_
