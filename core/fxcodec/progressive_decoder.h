// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_PROGRESSIVE_DECODER_H_
#define CORE_FXCODEC_PROGRESSIVE_DECODER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "core/fxcodec/fx_codec_def.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cstretchengine.h"
#include "core/fxge/dib/fx_dib.h"

#ifndef PDF_ENABLE_XFA
#error "XFA only"
#endif

class CFX_CodecMemory;
class CFX_DIBitmap;
class IFX_SeekableReadStream;

namespace fxcodec {

class CFX_DIBAttribute;
class ProgressiveDecoderContext;

class ProgressiveDecoder final : public ProgressiveDecoderContextDelegate {
 public:
  using Format = ProgressiveDecoderContextDelegate::Format;

  ProgressiveDecoder();
  ~ProgressiveDecoder() override;

  FXCODEC_STATUS LoadImageInfo(RetainPtr<IFX_SeekableReadStream> pFile,
                               FXCODEC_IMAGE_TYPE imageType,
                               CFX_DIBAttribute* pAttribute,
                               bool bSkipImageTypeCheck);

  int32_t GetWidth() const { return src_width_; }
  int32_t GetHeight() const { return src_height_; }

  FXDIB_Format GetBitmapFormat() const;

  std::pair<FXCODEC_STATUS, size_t> GetFrames();
  FXCODEC_STATUS StartDecode(RetainPtr<CFX_DIBitmap> bitmap);

  FXCODEC_STATUS ContinueDecode();

  // ProgressiveDecoderContextDelegate:
  bool ReadMoreData(std::optional<uint32_t> rcd_pos,
                    FXCODEC_STATUS* err_status) override;
  uint32_t GetCurrentInputPosition() const override;
  bool PrepareScanlineResampling(int src_width,
                                 int src_height,
                                 Format src_format,
                                 pdfium::span<const FX_ARGB> palette,
                                 std::optional<FX_ARGB> fill_argb) override;
  void ResampleScanline(int line,
                        pdfium::span<const uint8_t> src_span) override;
  bool PrepareDirectOutput(int src_width,
                           int src_height,
                           Format src_format) override;
  pdfium::span<uint8_t> AskScanlineBuf(int line) override;
  pdfium::span<uint8_t> AskImageBuf() override;

 private:
  using WeightTable = CStretchEngine::WeightTable;

  enum class TransformMethod : uint8_t {
    kInvalid,
    k8BppGrayToRgbMaybeAlpha,
    k8BppRgbToRgbNoAlpha,
    k8BppRgbToArgb,
    kRgbMaybeAlphaToRgbMaybeAlpha,
    kCmykToRgbMaybeAlpha,
    kArgbToArgb,
  };

  bool BmpReadMoreData(ProgressiveDecoderContext* bmp_context,
                       FXCODEC_STATUS* err_status);
  bool BmpDetectImageTypeInBuffer(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS BmpStartDecode();
  FXCODEC_STATUS BmpContinueDecode();

  bool GifReadMoreData(FXCODEC_STATUS* err_status);
  bool GifDetectImageTypeInBuffer();
  FXCODEC_STATUS GifStartDecode();
  FXCODEC_STATUS GifContinueDecode();

  bool PngDetectImageTypeInBuffer();
  FXCODEC_STATUS PngStartDecode();
  FXCODEC_STATUS PngContinueDecode();

  bool TiffDetectImageTypeFromFile(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS TiffContinueDecode();

  bool JpegReadMoreData(FXCODEC_STATUS* err_status);
  bool JpegDetectImageTypeInBuffer(CFX_DIBAttribute* pAttribute);
  FXCODEC_STATUS JpegStartDecode();
  FXCODEC_STATUS JpegContinueDecode();

  int32_t GetBitsPerPixel() const {
    return src_components_count_ * src_bits_per_component_;
  }

  bool DetectImageType(FXCODEC_IMAGE_TYPE imageType,
                       CFX_DIBAttribute* pAttribute);

  // Reads more data from `file_` into `codec_memory_`.
  //
  // Returns `false` and sets `err_status` upon failure.
  // Returns `true` to indicate success.
  //
  // Retains `unconsumed_bytes` at the end of `codec_memory_`.
  //
  // Reads start at `offset_` inside the file.  The `offset_` will be update as
  // needed.
  bool ReadMoreDataInternal(size_t unconsumed_bytes,
                            FXCODEC_STATUS* err_status);

  void SetTransMethod();

  void ResampleScanline(const RetainPtr<CFX_DIBitmap>& pDeviceBitmap,
                        int32_t dest_line,
                        pdfium::span<uint8_t> src_span,
                        Format src_format);
  void Resample(const RetainPtr<CFX_DIBitmap>& pDeviceBitmap,
                int32_t src_line,
                uint8_t* src_scan,
                Format src_format);

  // Computes the size of a single decoded image row (in bytes).
  //
  // This needs to be called *after* sufficient image metadata has been decoded
  // (i.e. `src_width_` and `src_components_count_` need to be known).
  int GetScanlineSize() const;

  FXCODEC_STATUS status_ = FXCODEC_STATUS::kDecodeFinished;
  FXCODEC_IMAGE_TYPE image_type_ = FXCODEC_IMAGE_UNKNOWN;
  RetainPtr<IFX_SeekableReadStream> file_;
  RetainPtr<CFX_DIBitmap> device_bitmap_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  DataVector<uint8_t> decode_buf_;
  DataVector<FX_ARGB> src_palette_;
  std::unique_ptr<ProgressiveDecoderContext> context_;
  uint32_t offset_ = 0;
  WeightTable weight_horz_;
  int src_width_ = 0;
  int src_height_ = 0;
  int src_components_count_ = 0;    // e.g. 4 for RGBA, or 3 for RGB
  int src_bits_per_component_ = 0;  // how many bits per channel
  TransformMethod trans_method_;
  Format src_format_ = Format::kInvalid;
  size_t frame_number_ = 0;
};

}  // namespace fxcodec

using ProgressiveDecoder = fxcodec::ProgressiveDecoder;

#endif  // CORE_FXCODEC_PROGRESSIVE_DECODER_H_
