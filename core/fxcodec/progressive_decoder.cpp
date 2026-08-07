// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/progressive_decoder.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "build/build_config.h"
#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/jpeg/libjpeg_jpeg_context.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_2d_size.h"
#include "core/fxcrt/fx_memcpy_wrappers.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/span_util.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/to_underlying.h"
#include "core/fxge/dib/cfx_cmyk_to_srgb.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"

#ifdef PDF_ENABLE_XFA_BMP
#if defined(PDF_ENABLE_RUST_BMP)
#include "core/fxcodec/bmp/skia_bmp_context.h"
#else
#include "core/fxcodec/bmp/cfx_bmpcontext.h"
#endif
#endif  // PDF_ENABLE_XFA_BMP

#ifdef PDF_ENABLE_XFA_GIF
#include "core/fxcodec/gif/cfx_gifcontext.h"
#endif  // PDF_ENABLE_XFA_GIF

#ifdef PDF_ENABLE_XFA_PNG
#include "core/fxcodec/png/png_decoder_delegate.h"
// TODO(https://crbug.com/444045690): Remove `pdf_enable_rust_png` from the
// condition below once this build mode has been tested and stabilized.
// (Chromium already sets `pdf_use_skia_override = true` so having an extra
// condition avoids affecting the Chromium behavior.)
#if defined(PDF_USE_SKIA) && defined(PDF_ENABLE_RUST_PNG)
#include "core/fxcodec/png/skia_png_context.h"
#else
#include "core/fxcodec/png/libpng_png_context.h"
#endif
#endif  // PDF_ENABLE_XFA_PNG

#ifdef PDF_ENABLE_XFA_TIFF
#include "core/fxcodec/tiff/libtiff_tiff_context.h"
#endif  // PDF_ENABLE_XFA_TIFF

namespace fxcodec {

namespace {

constexpr size_t kBlockSize = 4096;

std::unique_ptr<ProgressiveDecoderContext> CreateDecoderContext(
    FXCODEC_IMAGE_TYPE type,
    ProgressiveDecoder* delegate) {
  switch (type) {
#ifdef PDF_ENABLE_XFA_BMP
    case FXCODEC_IMAGE_BMP:
#if defined(PDF_ENABLE_RUST_BMP)
      return std::make_unique<SkiaBmpContext>(delegate);
#else
      return std::make_unique<CFX_BmpContext>(delegate);
#endif
#endif  // PDF_ENABLE_XFA_BMP
#ifdef PDF_ENABLE_XFA_GIF
    case FXCODEC_IMAGE_GIF:
      return std::make_unique<CFX_GifContext>(delegate);
#endif  // PDF_ENABLE_XFA_GIF
    case FXCODEC_IMAGE_JPG: {
      auto context = std::make_unique<LibjpegJpegContext>();
      if (!context->create_ok_) {
        return nullptr;
      }
      return context;
    }
#ifdef PDF_ENABLE_XFA_PNG
    case FXCODEC_IMAGE_PNG:
#if defined(PDF_USE_SKIA) && defined(PDF_ENABLE_RUST_PNG)
      return std::make_unique<SkiaPngContext>(delegate);
#else
    {
      auto context = std::make_unique<LibpngPngContext>(delegate);
      if (!context->png_ || !context->info_) {
        return nullptr;
      }
      return context;
    }
#endif
#endif  // PDF_ENABLE_XFA_PNG
    default:
      return nullptr;
  }
}

#ifdef PDF_ENABLE_XFA_PNG
#if BUILDFLAG(IS_APPLE)
const double kPngGamma = 1.7;
#else
const double kPngGamma = 2.2;
#endif  // BUILDFLAG(IS_APPLE)
#endif  // PDF_ENABLE_XFA_PNG

void RGB2BGR(uint8_t* buffer, int width = 1) {
  if (buffer && width > 0) {
    uint8_t temp;
    int i = 0;
    int j = 0;
    UNSAFE_TODO({
      for (; i < width; i++, j += 3) {
        temp = buffer[j];
        buffer[j] = buffer[j + 2];
        buffer[j + 2] = temp;
      }
    });
  }
}

}  // namespace

ProgressiveDecoder::ProgressiveDecoder() = default;

ProgressiveDecoder::~ProgressiveDecoder() = default;

#ifdef PDF_ENABLE_XFA_PNG
bool ProgressiveDecoder::PngReadHeader(int width, int height, double* gamma) {
  if (!device_bitmap_) {
    got_png_metadata_ = true;
    src_width_ = width;
    src_height_ = height;

    // PNG decoder always decodes into BGRA.
    src_bits_per_component_ = 8;
    src_components_count_ = 4;
    src_format_ = Format::kArgb;

    return false;
  }

  CHECK_EQ(width, src_width_);
  CHECK_EQ(height, src_height_);
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  *gamma = kPngGamma;
  return true;
}

pdfium::span<uint8_t> ProgressiveDecoder::PngAskScanlineBuf(int line) {
  CHECK_GE(line, 0);
  CHECK_LT(line, src_height_);
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  CHECK_EQ(src_format_, Format::kArgb);
  return device_bitmap_->GetWritableScanline(line);
}

pdfium::span<uint8_t> ProgressiveDecoder::PngAskImageBuf() {
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  CHECK_EQ(src_format_, Format::kArgb);
  return device_bitmap_->GetWritableBuffer();
}

void ProgressiveDecoder::PngFinishedDecoding() {
  status_ = FXCODEC_STATUS::kDecodeFinished;
}
#endif  // PDF_ENABLE_XFA_PNG

#ifdef PDF_ENABLE_XFA_GIF
uint32_t ProgressiveDecoder::GifCurrentPosition() const {
  uint32_t remain_size =
      pdfium::checked_cast<uint32_t>(context_->GetAvailInput());
  return offset_ - remain_size;
}

bool ProgressiveDecoder::GifInputRecordPositionBuf(
    uint32_t rcd_pos,
    const FX_RECT& img_rc,
    pdfium::span<CFX_GifPalette> pal_span,
    int32_t trans_index) {
  offset_ = rcd_pos;

  FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
  codec_memory_->Seek(codec_memory_->GetSize());
  if (!GifReadMoreData(&error_status)) {
    return false;
  }

  if (pal_span.empty()) {
    pal_span = gif_palette_;
  }
  if (pal_span.empty()) {
    return false;
  }
  src_palette_.resize(pal_span.size());
  for (size_t i = 0; i < pal_span.size(); i++) {
    src_palette_[i] =
        ArgbEncode(0xff, pal_span[i].r, pal_span[i].g, pal_span[i].b);
  }
  gif_trans_index_ = trans_index;
  gif_frame_rect_ = img_rc;
  int32_t pal_index = gif_bg_index_;
  RetainPtr<CFX_DIBitmap> pDevice = device_bitmap_;
  if (trans_index >= static_cast<int>(pal_span.size())) {
    trans_index = -1;
  }
  if (trans_index != -1) {
    src_palette_[trans_index] &= 0x00ffffff;
    if (pDevice->IsAlphaFormat()) {
      pal_index = trans_index;
    }
  }
  if (pal_index >= static_cast<int>(pal_span.size())) {
    return false;
  }
  int startX = 0;
  int startY = 0;
  int sizeX = src_width_;
  int sizeY = src_height_;
  const int bytes_per_pixel = pDevice->GetBPP() / 8;
  FX_ARGB argb = src_palette_[pal_index];
  for (int row = 0; row < sizeY; row++) {
    pdfium::span<uint8_t> scan_span =
        pDevice->GetWritableScanline(row + startY)
            .subspan(static_cast<size_t>(startX * bytes_per_pixel));
    switch (trans_method_) {
      case TransformMethod::k8BppRgbToRgbNoAlpha: {
        uint8_t* pScanline = scan_span.data();
        UNSAFE_TODO({
          for (int col = 0; col < sizeX; col++) {
            *pScanline++ = FXARGB_B(argb);
            *pScanline++ = FXARGB_G(argb);
            *pScanline++ = FXARGB_R(argb);
            pScanline += bytes_per_pixel - 3;
          }
        });
        break;
      }
      case TransformMethod::k8BppRgbToArgb: {
        for (int col = 0; col < sizeX; col++) {
          FXARGB_SetDIB(scan_span.first<4u>(), argb);
          scan_span = scan_span.subspan<4u>();
        }
        break;
      }
      default:
        break;
    }
  }
  return true;
}

void ProgressiveDecoder::GifReadScanline(int32_t row_num,
                                         pdfium::span<uint8_t> row_buf) {
  RetainPtr<CFX_DIBitmap> pDIBitmap = device_bitmap_;
  const size_t img_width = static_cast<size_t>(gif_frame_rect_.Width());
  const pdfium::span<uint8_t> row_span = row_buf.first(img_width);
  if (!pDIBitmap->IsAlphaFormat()) {
    for (auto& byte_ref : row_span) {
      if (byte_ref == gif_trans_index_) {
        byte_ref = gif_bg_index_;
      }
    }
  }
  int32_t pal_index = gif_bg_index_;
  if (gif_trans_index_ != -1 && device_bitmap_->IsAlphaFormat()) {
    pal_index = gif_trans_index_;
  }
  const int32_t left = gif_frame_rect_.left;
  const pdfium::span<uint8_t> decode_span = decode_buf_;
  std::ranges::fill(decode_span.first(static_cast<size_t>(src_width_)),
                    pal_index);
  fxcrt::Copy(row_span, decode_span.subspan(static_cast<size_t>(left)));
  int32_t line = row_num + gif_frame_rect_.top;
  if (line < 0 || line >= src_height_) {
    return;
  }
  ResampleScanline(pDIBitmap, line, decode_span, src_format_);
}
#endif  // PDF_ENABLE_XFA_GIF

bool ProgressiveDecoder::ReadMoreData(std::optional<uint32_t> rcd_pos,
                                      FXCODEC_STATUS* err_status) {
  if (rcd_pos.has_value()) {
    offset_ = rcd_pos.value();
  }
  size_t unconsumed_bytes = codec_memory_->GetUnconsumedSpan().size();
  if (!ReadMoreDataInternal(unconsumed_bytes, err_status)) {
    return false;
  }
  if (context_) {
    context_->Input(codec_memory_);
  }
  return true;
}

uint32_t ProgressiveDecoder::GetCurrentInputPosition() const {
  return offset_;
}

bool ProgressiveDecoder::PrepareScanlineResampling(
    int src_width,
    int src_height,
    Format src_format,
    pdfium::span<const FX_ARGB> palette,
    std::optional<FX_ARGB> fill_argb) {
  if (!device_bitmap_) {
    return false;
  }
  src_width_ = src_width;
  src_height_ = src_height;
  src_format_ = src_format;
  // For formats other than kArgb, these values are already reliably set
  // during the header reading phase.
  if (src_format_ == Format::kArgb) {
    src_bits_per_component_ = 8;
    src_components_count_ = 4;
  }
  SetTransMethod();
  decode_buf_.resize(GetScanlineSize());
  FXDIB_ResampleOptions options;
  options.bInterpolateBilinear = true;
  weight_horz_.CalculateWeights(src_width_, 0, src_width_, src_width_, 0,
                                src_width_, options);

  if (!palette.empty()) {
    device_bitmap_->SetPalette(palette);
  }
  if (fill_argb.has_value()) {
    device_bitmap_->Clear(fill_argb.value());
  }
  return true;
}

void ProgressiveDecoder::ResampleScanline(
    int line,
    pdfium::span<const uint8_t> src_span) {
  CHECK(device_bitmap_);
  if (line < 0 || line >= src_height_) {
    return;
  }
  int scanline_size = GetScanlineSize();
  fxcrt::Copy(src_span.first(static_cast<size_t>(scanline_size)), decode_buf_);
  ResampleScanline(device_bitmap_, line, decode_buf_, src_format_);
}

pdfium::span<uint8_t> ProgressiveDecoder::AskScanlineBuf(int line) {
  CHECK_GE(line, 0);
  CHECK_LT(line, src_height_);
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  CHECK_EQ(src_format_, Format::kArgb);
  return device_bitmap_->GetWritableScanline(line);
}

pdfium::span<uint8_t> ProgressiveDecoder::AskImageBuf() {
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  CHECK_EQ(src_format_, Format::kArgb);
  return device_bitmap_->GetWritableBuffer();
}

#ifdef PDF_ENABLE_XFA_BMP

bool ProgressiveDecoder::BmpDetectImageTypeInBuffer(
    CFX_DIBAttribute* pAttribute) {
  std::unique_ptr<ProgressiveDecoderContext> pBmcontext =
      CreateDecoderContext(FXCODEC_IMAGE_BMP, this);
  pBmcontext->Input(codec_memory_);

#if defined(PDF_ENABLE_RUST_BMP)
  auto* ctx = static_cast<SkiaBmpContext*>(pBmcontext.get());
#else
  auto* ctx = static_cast<CFX_BmpContext*>(pBmcontext.get());
#endif

  pdfium::span<const FX_ARGB> palette;
  ProgressiveDecoderContext::Status read_result = ctx->ReadHeader(
      &src_width_, &src_height_, &src_components_count_, &palette, pAttribute);
  while (read_result == ProgressiveDecoderContext::Status::kContinue) {
    FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
    if (!BmpReadMoreData(pBmcontext.get(), &error_status)) {
      status_ = error_status;
      return false;
    }
    read_result = ctx->ReadHeader(&src_width_, &src_height_,
                                  &src_components_count_, &palette, pAttribute);
  }

  if (read_result != ProgressiveDecoderContext::Status::kSuccess) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }

  FXDIB_Format format = FXDIB_Format::kInvalid;
  switch (src_components_count_) {
    case 1:
      src_format_ = Format::k8bppRgb;
      format = FXDIB_Format::k8bppRgb;
      break;
    case 3:
      src_format_ = Format::kRgb;
      format = FXDIB_Format::kBgr;
      break;
    case 4:
      src_format_ = Format::kRgb32;
      format = FXDIB_Format::kBgrx;
      break;
    default:
      status_ = FXCODEC_STATUS::kError;
      return false;
  }

  // Set to 0 to make CalculatePitchAndSize() calculate it.
  static constexpr uint32_t kNoPitch = 0;
  std::optional<CFX_DIBitmap::PitchAndSize> needed_data =
      CFX_DIBitmap::CalculatePitchAndSize(src_width_, src_height_, format,
                                          kNoPitch);
  if (!needed_data.has_value()) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }

  uint32_t available_data = pdfium::checked_cast<uint32_t>(
      file_->GetSize() - offset_ + pBmcontext->GetAvailInput());
  if (needed_data.value().size / src_components_count_ > available_data) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }

  src_bits_per_component_ = 8;
  context_ = std::move(pBmcontext);
  if (!palette.empty()) {
    src_palette_.resize(palette.size());
    fxcrt::Copy(palette, src_palette_);
  } else {
    src_palette_.clear();
  }
  return true;
}

bool ProgressiveDecoder::BmpReadMoreData(ProgressiveDecoderContext* bmp_context,
                                         FXCODEC_STATUS* err_status) {
  // TODO(lukasza): Can this just use
  // `codec_memory_->GetUnconsumedSpan().size()`? (IIUC this is what
  // `GetAvailInput` uses in the end, but I haven't investigated that this is
  // the same instance of `CFX_CodecMemory`.)
  FX_SAFE_SIZE_T avail_input = bmp_context->GetAvailInput();
  if (!avail_input.IsValid()) {
    return false;
  }
  if (!ReadMoreDataInternal(avail_input.ValueOrDie(), err_status)) {
    return false;
  }
  bmp_context->Input(codec_memory_);
  return true;
}

FXCODEC_STATUS ProgressiveDecoder::BmpStartDecode() {
  SetTransMethod();
  decode_buf_.resize(GetScanlineSize());
  FXDIB_ResampleOptions options;
  options.bInterpolateBilinear = true;
  weight_horz_.CalculateWeights(src_width_, 0, src_width_, src_width_, 0,
                                src_width_, options);
  status_ = FXCODEC_STATUS::kDecodeToBeContinued;
  return status_;
}

FXCODEC_STATUS ProgressiveDecoder::BmpContinueDecode() {
  ProgressiveDecoderContext::Status read_res = context_->DecodeImage(0);
  while (read_res == ProgressiveDecoderContext::Status::kContinue) {
    FXCODEC_STATUS error_status = FXCODEC_STATUS::kDecodeFinished;
    if (!BmpReadMoreData(context_.get(), &error_status)) {
      device_bitmap_ = nullptr;
      file_ = nullptr;
      status_ = error_status;
      return status_;
    }
    read_res = context_->DecodeImage(0);
  }

  device_bitmap_ = nullptr;
  file_ = nullptr;
  status_ = read_res == ProgressiveDecoderContext::Status::kSuccess
                ? FXCODEC_STATUS::kDecodeFinished
                : FXCODEC_STATUS::kError;
  return status_;
}
#endif  // PDF_ENABLE_XFA_BMP

#ifdef PDF_ENABLE_XFA_GIF
bool ProgressiveDecoder::GifReadMoreData(FXCODEC_STATUS* err_status) {
  // TODO(lukasza): Can this just use
  // `codec_memory_->GetUnconsumedSpan().size()`? (IIUC this is what
  // `GetAvailInput` uses in the end, but I haven't investigated that this is
  // the same instance of `CFX_CodecMemory`.)
  FX_SAFE_SIZE_T avail_input = context_->GetAvailInput();
  if (!avail_input.IsValid()) {
    return false;
  }
  if (!ReadMoreDataInternal(avail_input.ValueOrDie(), err_status)) {
    return false;
  }

  context_->Input(codec_memory_);
  return true;
}

bool ProgressiveDecoder::GifDetectImageTypeInBuffer() {
  context_ = CreateDecoderContext(FXCODEC_IMAGE_GIF, this);
  context_->Input(codec_memory_);
  src_components_count_ = 1;
  auto* ctx = static_cast<CFX_GifContext*>(context_.get());
  ProgressiveDecoderContext::Status readResult =
      ctx->ReadHeader(&src_width_, &src_height_, &gif_palette_, &gif_bg_index_);
  while (readResult == ProgressiveDecoderContext::Status::kContinue) {
    FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
    if (!GifReadMoreData(&error_status)) {
      context_ = nullptr;
      status_ = error_status;
      return false;
    }
    readResult = ctx->ReadHeader(&src_width_, &src_height_, &gif_palette_,
                                 &gif_bg_index_);
  }
  if (readResult == ProgressiveDecoderContext::Status::kSuccess) {
    src_bits_per_component_ = 8;
    return true;
  }
  context_ = nullptr;
  status_ = FXCODEC_STATUS::kError;
  return false;
}

FXCODEC_STATUS ProgressiveDecoder::GifStartDecode() {
  src_format_ = Format::k8bppRgb;
  SetTransMethod();
  decode_buf_.resize(GetScanlineSize());
  FXDIB_ResampleOptions options;
  options.bInterpolateBilinear = true;
  weight_horz_.CalculateWeights(src_width_, 0, src_width_, src_width_, 0,
                                src_width_, options);
  frame_cur_ = 0;
  status_ = FXCODEC_STATUS::kDecodeToBeContinued;
  return status_;
}

FXCODEC_STATUS ProgressiveDecoder::GifContinueDecode() {
  ProgressiveDecoderContext::Status readRes = context_->DecodeImage(frame_cur_);
  while (readRes == ProgressiveDecoderContext::Status::kContinue) {
    FXCODEC_STATUS error_status = FXCODEC_STATUS::kDecodeFinished;
    if (!GifReadMoreData(&error_status)) {
      device_bitmap_ = nullptr;
      file_ = nullptr;
      status_ = error_status;
      return status_;
    }
    readRes = context_->DecodeImage(frame_cur_);
  }

  if (readRes == ProgressiveDecoderContext::Status::kSuccess) {
    device_bitmap_ = nullptr;
    file_ = nullptr;
    status_ = FXCODEC_STATUS::kDecodeFinished;
    return status_;
  }

  device_bitmap_ = nullptr;
  file_ = nullptr;
  status_ = FXCODEC_STATUS::kError;
  return status_;
}
#endif  // PDF_ENABLE_XFA_GIF

bool ProgressiveDecoder::JpegReadMoreData(FXCODEC_STATUS* err_status) {
  FX_SAFE_SIZE_T avail_input = context_->GetAvailInput();
  if (!avail_input.IsValid()) {
    return false;
  }
  if (!ReadMoreDataInternal(avail_input.ValueOrDie(), err_status)) {
    return false;
  }
  context_->Input(codec_memory_);
  return true;
}

bool ProgressiveDecoder::JpegDetectImageTypeInBuffer(
    CFX_DIBAttribute* pAttribute) {
  context_ = CreateDecoderContext(FXCODEC_IMAGE_JPG, this);
  if (!context_) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }
  context_->Input(codec_memory_);

  auto* ctx = static_cast<LibjpegJpegContext*>(context_.get());
  while (1) {
    int read_result = ctx->ReadHeader(&src_width_, &src_height_,
                                      &src_components_count_, pAttribute);
    switch (read_result) {
      case LibjpegJpegContext::kFatal:
      case LibjpegJpegContext::kError:
        status_ = FXCODEC_STATUS::kError;
        return false;
      case LibjpegJpegContext::kOk:
        src_bits_per_component_ = 8;
        return true;
      case LibjpegJpegContext::kNeedsMoreInput: {
        FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
        if (!JpegReadMoreData(&error_status)) {
          status_ = error_status;
          return false;
        }
        break;
      }
      default:
        NOTREACHED();
    }
  }
}

FXCODEC_STATUS ProgressiveDecoder::JpegStartDecode() {
  auto* ctx = static_cast<LibjpegJpegContext*>(context_.get());
  while (!ctx->StartScanline()) {
    // Maybe it needs more data.
    FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
    if (!JpegReadMoreData(&error_status)) {
      device_bitmap_ = nullptr;
      file_ = nullptr;
      status_ = error_status;
      return status_;
    }
  }
  decode_buf_.resize(GetScanlineSize());
  FXDIB_ResampleOptions options;
  options.bInterpolateBilinear = true;
  weight_horz_.CalculateWeights(src_width_, 0, src_width_, src_width_, 0,
                                src_width_, options);
  switch (src_components_count_) {
    case 1:
      src_format_ = Format::k8bppGray;
      break;
    case 3:
      src_format_ = Format::kRgb;
      break;
    case 4:
      src_format_ = Format::kCmyk;
      break;
  }
  SetTransMethod();
  status_ = FXCODEC_STATUS::kDecodeToBeContinued;
  return status_;
}

FXCODEC_STATUS ProgressiveDecoder::JpegContinueDecode() {
  auto* ctx = static_cast<LibjpegJpegContext*>(context_.get());
  while (true) {
    int err_code = ctx->ReadScanline(decode_buf_.data());
    if (err_code == LibjpegJpegContext::kFatal) {
      context_.reset();
      status_ = FXCODEC_STATUS::kError;
      return FXCODEC_STATUS::kError;
    }
    if (err_code != LibjpegJpegContext::kOk) {
      // Maybe it needs more data.
      FXCODEC_STATUS error_status = FXCODEC_STATUS::kDecodeFinished;
      if (JpegReadMoreData(&error_status)) {
        continue;
      }
      device_bitmap_ = nullptr;
      file_ = nullptr;
      status_ = error_status;
      return status_;
    }
    if (src_format_ == Format::kRgb) {
      RGB2BGR(UNSAFE_TODO(decode_buf_.data()), src_width_);
    }
    if (src_row_ >= src_height_) {
      device_bitmap_ = nullptr;
      file_ = nullptr;
      status_ = FXCODEC_STATUS::kDecodeFinished;
      return status_;
    }
    Resample(device_bitmap_, src_row_, decode_buf_.data(), src_format_);
    src_row_++;
  }
}

#ifdef PDF_ENABLE_XFA_PNG
bool ProgressiveDecoder::PngReadMoreData() {
  size_t unconsumed_bytes = codec_memory_->GetUnconsumedSpan().size();
  if (!ReadMoreDataInternal(unconsumed_bytes, &status_)) {
    return false;
  }

#if defined(PDF_USE_SKIA) && defined(PDF_ENABLE_RUST_PNG)
  auto* ctx = static_cast<SkiaPngContext*>(context_.get());
#else
  auto* ctx = static_cast<LibpngPngContext*>(context_.get());
#endif
  return ctx->ContinueDecode(codec_memory_);
}

bool ProgressiveDecoder::PngDetectImageTypeInBuffer() {
  context_ = CreateDecoderContext(FXCODEC_IMAGE_PNG, this);
  if (!context_) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }

#if defined(PDF_USE_SKIA) && defined(PDF_ENABLE_RUST_PNG)
  auto* ctx = static_cast<SkiaPngContext*>(context_.get());
#else
  auto* ctx = static_cast<LibpngPngContext*>(context_.get());
#endif

  // Keep feeding more input into the decoder until either the decoder 1) fails,
  // or 2) calls `PngReadHeader` to indicate that it `got_png_metadata_`.
  if (ctx->ContinueDecode(codec_memory_)) {
    while (!got_png_metadata_ && PngReadMoreData()) {
    }
  }

  // Return `got_png_metadata_` and ignore any failures that the decoder may
  // have reported.  (In particular ignore the failure that `PngReadHeader`
  // reports when there is no `device_bitmap_` - e.g. during image type
  // detection.)
  context_.reset();
  return got_png_metadata_;
}

FXCODEC_STATUS ProgressiveDecoder::PngStartDecode() {
  context_ = CreateDecoderContext(FXCODEC_IMAGE_PNG, this);
  if (!context_) {
    device_bitmap_ = nullptr;
    file_ = nullptr;
    status_ = FXCODEC_STATUS::kError;
    return status_;
  }

  // No need to resample/transform pixels when decoding PNGs, because 1)
  // `device_bitmap_` for PNGs is always kBgra and 2) the decoder always outputs
  // `Format::kArgb` (the same format).  In other words, PNG code path doesn't
  // need to use an intermediate `decode_buf_` to transform the pixels via
  // `SetTransMethod` and `ResampleScanline`.
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  CHECK_EQ(src_format_, Format::kArgb);

  // Discard old/stale data from `codec_memory_` and restart reading the `file_`
  // from `offset_` 0.
  codec_memory_->Seek(codec_memory_->GetSize());
  offset_ = 0;

  status_ = FXCODEC_STATUS::kDecodeToBeContinued;
  return status_;
}

FXCODEC_STATUS ProgressiveDecoder::PngContinueDecode() {
  while (status_ == FXCODEC_STATUS::kDecodeToBeContinued) {
    if (!PngReadMoreData()) {
      status_ = FXCODEC_STATUS::kError;
      break;
    }
  }

  context_.reset();
  device_bitmap_ = nullptr;
  file_ = nullptr;
  CHECK(status_ == FXCODEC_STATUS::kDecodeFinished ||
        status_ == FXCODEC_STATUS::kError);
  return status_;
}
#endif  // PDF_ENABLE_XFA_PNG

#ifdef PDF_ENABLE_XFA_TIFF
bool ProgressiveDecoder::TiffDetectImageTypeFromFile(
    CFX_DIBAttribute* pAttribute) {
  auto ctx = std::make_unique<LibtiffTiffContext>();
  if (!ctx->InitDecoder(file_)) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }
  int32_t dummy_bpc;
  bool ret = ctx->LoadFrameInfo(0, &src_width_, &src_height_,
                                &src_components_count_, &dummy_bpc, pAttribute);
  src_components_count_ = 4;
  if (!ret) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }
  context_ = std::move(ctx);
  return true;
}

FXCODEC_STATUS ProgressiveDecoder::TiffContinueDecode() {
  // TODO(crbug.com/355630556): Consider adding support for
  // `FXDIB_Format::kBgraPremul`
  CHECK_EQ(device_bitmap_->GetFormat(), FXDIB_Format::kBgra);
  auto* ctx = static_cast<LibtiffTiffContext*>(context_.get());
  status_ = ctx->Decode(std::move(device_bitmap_))
                ? FXCODEC_STATUS::kDecodeFinished
                : FXCODEC_STATUS::kError;
  file_ = nullptr;
  return status_;
}
#endif  // PDF_ENABLE_XFA_TIFF

bool ProgressiveDecoder::DetectImageType(FXCODEC_IMAGE_TYPE imageType,
                                         CFX_DIBAttribute* pAttribute) {
#ifdef PDF_ENABLE_XFA_TIFF
  if (imageType == FXCODEC_IMAGE_TIFF) {
    return TiffDetectImageTypeFromFile(pAttribute);
  }
#endif  // PDF_ENABLE_XFA_TIFF

  size_t size = pdfium::checked_cast<size_t>(
      std::min<FX_FILESIZE>(file_->GetSize(), kBlockSize));
  codec_memory_ = pdfium::MakeRetain<CFX_CodecMemory>(size);
  offset_ = 0;
  if (!file_->ReadBlockAtOffset(codec_memory_->GetBufferSpan().first(size),
                                offset_)) {
    status_ = FXCODEC_STATUS::kError;
    return false;
  }
  offset_ += size;

  if (imageType == FXCODEC_IMAGE_JPG) {
    return JpegDetectImageTypeInBuffer(pAttribute);
  }

#ifdef PDF_ENABLE_XFA_BMP
  if (imageType == FXCODEC_IMAGE_BMP) {
    return BmpDetectImageTypeInBuffer(pAttribute);
  }
#endif  // PDF_ENABLE_XFA_BMP

#ifdef PDF_ENABLE_XFA_GIF
  if (imageType == FXCODEC_IMAGE_GIF) {
    return GifDetectImageTypeInBuffer();
  }
#endif  // PDF_ENABLE_XFA_GIF

#ifdef PDF_ENABLE_XFA_PNG
  if (imageType == FXCODEC_IMAGE_PNG) {
    return PngDetectImageTypeInBuffer();
  }
#endif  // PDF_ENABLE_XFA_PNG

  status_ = FXCODEC_STATUS::kError;
  return false;
}

bool ProgressiveDecoder::ReadMoreDataInternal(size_t unconsumed_bytes,
                                              FXCODEC_STATUS* err_status) {
  // Check for EOF.
  if (offset_ >= static_cast<uint32_t>(file_->GetSize())) {
    return false;
  }

  // Try to get whatever remains.
  uint32_t bytes_to_fetch_from_file =
      pdfium::checked_cast<uint32_t>(file_->GetSize() - offset_);

  if (unconsumed_bytes == codec_memory_->GetSize()) {
    // Codec couldn't make any progress against the bytes in the buffer.
    // Increase the buffer size so that there might be enough contiguous
    // bytes to allow whatever operation is having difficulty to succeed.
    bytes_to_fetch_from_file =
        std::min<uint32_t>(bytes_to_fetch_from_file, kBlockSize);
    size_t new_size = codec_memory_->GetSize() + bytes_to_fetch_from_file;
    if (!codec_memory_->TryResize(new_size)) {
      *err_status = FXCODEC_STATUS::kError;
      return false;
    }
  } else {
    // TODO(crbug.com/42270919): Simplify the `CFX_CodecMemory` API so we
    // don't need to do this awkward dance to free up exactly enough buffer
    // space for the next read.
    size_t already_read_bytes = codec_memory_->GetSize() - unconsumed_bytes;
    bytes_to_fetch_from_file = pdfium::checked_cast<uint32_t>(
        std::min<size_t>(bytes_to_fetch_from_file, already_read_bytes));
    codec_memory_->Consume(bytes_to_fetch_from_file);
    codec_memory_->Seek(already_read_bytes - bytes_to_fetch_from_file);
    unconsumed_bytes += codec_memory_->GetPosition();
  }

  // Append new data past the bytes not yet processed by the codec.
  if (!file_->ReadBlockAtOffset(codec_memory_->GetBufferSpan().subspan(
                                    unconsumed_bytes, bytes_to_fetch_from_file),
                                offset_)) {
    *err_status = FXCODEC_STATUS::kError;
    return false;
  }
  offset_ += bytes_to_fetch_from_file;
  return true;
}

FXCODEC_STATUS ProgressiveDecoder::LoadImageInfo(
    RetainPtr<IFX_SeekableReadStream> pFile,
    FXCODEC_IMAGE_TYPE imageType,
    CFX_DIBAttribute* pAttribute,
    bool bSkipImageTypeCheck) {
  DCHECK(pAttribute);

  switch (status_) {
    case FXCODEC_STATUS::kFrameReady:
    case FXCODEC_STATUS::kFrameToBeContinued:
    case FXCODEC_STATUS::kDecodeReady:
    case FXCODEC_STATUS::kDecodeToBeContinued:
      return FXCODEC_STATUS::kError;
    case FXCODEC_STATUS::kError:
    case FXCODEC_STATUS::kDecodeFinished:
      break;
  }
  file_ = std::move(pFile);
  if (!file_) {
    status_ = FXCODEC_STATUS::kError;
    return status_;
  }
  offset_ = 0;
  src_width_ = 0;
  src_height_ = 0;
  src_components_count_ = 0;
  src_bits_per_component_ = 0;
  if (imageType != FXCODEC_IMAGE_UNKNOWN &&
      DetectImageType(imageType, pAttribute)) {
    image_type_ = imageType;
    status_ = FXCODEC_STATUS::kFrameReady;
    return status_;
  }
  // If we got here then the image data does not match the requested decoder.
  // If we're skipping the type check then bail out at this point and return
  // the failed status.
  if (bSkipImageTypeCheck) {
    return status_;
  }

  for (int type = FXCODEC_IMAGE_UNKNOWN + 1; type < FXCODEC_IMAGE_MAX; type++) {
    if (DetectImageType(static_cast<FXCODEC_IMAGE_TYPE>(type), pAttribute)) {
      image_type_ = static_cast<FXCODEC_IMAGE_TYPE>(type);
      status_ = FXCODEC_STATUS::kFrameReady;
      return status_;
    }
  }
  status_ = FXCODEC_STATUS::kError;
  file_ = nullptr;
  return status_;
}

void ProgressiveDecoder::SetTransMethod() {
  switch (device_bitmap_->GetFormat()) {
    case FXDIB_Format::kInvalid:
    case FXDIB_Format::k1bppMask:
    case FXDIB_Format::k1bppRgb:
    case FXDIB_Format::k8bppMask:
    case FXDIB_Format::k8bppRgb:
      NOTREACHED();
    case FXDIB_Format::kBgr: {
      switch (src_format_) {
        case Format::kInvalid:
          trans_method_ = TransformMethod::kInvalid;
          break;
        case Format::k8bppGray:
          trans_method_ = TransformMethod::k8BppGrayToRgbMaybeAlpha;
          break;
        case Format::k8bppRgb:
          trans_method_ = TransformMethod::k8BppRgbToRgbNoAlpha;
          break;
        case Format::kRgb:
        case Format::kRgb32:
        case Format::kArgb:
          trans_method_ = TransformMethod::kRgbMaybeAlphaToRgbMaybeAlpha;
          break;
        case Format::kCmyk:
          trans_method_ = TransformMethod::kCmykToRgbMaybeAlpha;
          break;
      }
      break;
    }
    case FXDIB_Format::kBgrx:
    case FXDIB_Format::kBgra: {
      switch (src_format_) {
        case Format::kInvalid:
          trans_method_ = TransformMethod::kInvalid;
          break;
        case Format::k8bppGray:
          trans_method_ = TransformMethod::k8BppGrayToRgbMaybeAlpha;
          break;
        case Format::k8bppRgb:
          if (device_bitmap_->GetFormat() == FXDIB_Format::kBgra) {
            trans_method_ = TransformMethod::k8BppRgbToArgb;
          } else {
            trans_method_ = TransformMethod::k8BppRgbToRgbNoAlpha;
          }
          break;
        case Format::kRgb:
        case Format::kRgb32:
          trans_method_ = TransformMethod::kRgbMaybeAlphaToRgbMaybeAlpha;
          break;
        case Format::kCmyk:
          trans_method_ = TransformMethod::kCmykToRgbMaybeAlpha;
          break;
        case Format::kArgb:
          trans_method_ = TransformMethod::kArgbToArgb;
          break;
      }
      break;
    }
#if defined(PDF_USE_SKIA)
    case FXDIB_Format::kBgraPremul:
      // TODO(crbug.com/355630556): Consider adding support for
      // `FXDIB_Format::kBgraPremul`
      NOTREACHED();
#endif
  }
}

void ProgressiveDecoder::ResampleScanline(
    const RetainPtr<CFX_DIBitmap>& pDeviceBitmap,
    int dest_line,
    pdfium::span<uint8_t> src_span,
    Format src_format) {
  uint8_t* src_scan = src_span.data();
  uint8_t* dest_scan = pDeviceBitmap->GetWritableScanline(dest_line).data();
  const auto src_bytes_per_pixel =
      (fxcrt::to_underlying(src_format) & 0xff) / 8;
  const int dest_bytes_per_pixel = pDeviceBitmap->GetBPP() / 8;
  for (int dest_col = 0; dest_col < src_width_; dest_col++) {
    CStretchEngine::PixelWeight* pPixelWeights =
        weight_horz_.GetPixelWeight(dest_col);
    switch (trans_method_) {
      case TransformMethod::kInvalid:
        return;
      case TransformMethod::k8BppGrayToRgbMaybeAlpha: {
        UNSAFE_TODO({
          uint32_t dest_g = 0;
          for (int j = pPixelWeights->src_start_; j <= pPixelWeights->src_end_;
               j++) {
            uint32_t pixel_weight =
                pPixelWeights->weights_[j - pPixelWeights->src_start_];
            dest_g += pixel_weight * src_scan[j];
          }
          FXSYS_memset(dest_scan, CStretchEngine::PixelFromFixed(dest_g), 3);
          dest_scan += dest_bytes_per_pixel;
          break;
        });
      }
      case TransformMethod::k8BppRgbToRgbNoAlpha: {
        UNSAFE_TODO({
          uint32_t dest_r = 0;
          uint32_t dest_g = 0;
          uint32_t dest_b = 0;
          for (int j = pPixelWeights->src_start_; j <= pPixelWeights->src_end_;
               j++) {
            uint32_t pixel_weight =
                pPixelWeights->weights_[j - pPixelWeights->src_start_];
            uint32_t argb = src_palette_[src_scan[j]];
            dest_r += pixel_weight * FXARGB_R(argb);
            dest_g += pixel_weight * FXARGB_G(argb);
            dest_b += pixel_weight * FXARGB_B(argb);
          }
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_b);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_g);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_r);
          dest_scan += dest_bytes_per_pixel - 3;
          break;
        });
      }
      case TransformMethod::k8BppRgbToArgb: {
        UNSAFE_TODO({
          uint32_t dest_a = 0;
          uint32_t dest_r = 0;
          uint32_t dest_g = 0;
          uint32_t dest_b = 0;
          for (int j = pPixelWeights->src_start_; j <= pPixelWeights->src_end_;
               j++) {
            uint32_t pixel_weight =
                pPixelWeights->weights_[j - pPixelWeights->src_start_];
            FX_ARGB argb = src_palette_[src_scan[j]];
            dest_a += pixel_weight * FXARGB_A(argb);
            dest_r += pixel_weight * FXARGB_R(argb);
            dest_g += pixel_weight * FXARGB_G(argb);
            dest_b += pixel_weight * FXARGB_B(argb);
          }
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_b);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_g);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_r);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_a);
          break;
        });
      }
      case TransformMethod::kRgbMaybeAlphaToRgbMaybeAlpha: {
        UNSAFE_TODO({
          uint32_t dest_b = 0;
          uint32_t dest_g = 0;
          uint32_t dest_r = 0;
          for (int j = pPixelWeights->src_start_; j <= pPixelWeights->src_end_;
               j++) {
            uint32_t pixel_weight =
                pPixelWeights->weights_[j - pPixelWeights->src_start_];
            const uint8_t* src_pixel = src_scan + j * src_bytes_per_pixel;
            dest_b += pixel_weight * (*src_pixel++);
            dest_g += pixel_weight * (*src_pixel++);
            dest_r += pixel_weight * (*src_pixel);
          }
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_b);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_g);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_r);
          dest_scan += dest_bytes_per_pixel - 3;
          break;
        });
      }
      case TransformMethod::kCmykToRgbMaybeAlpha: {
        UNSAFE_TODO({
          uint32_t dest_b = 0;
          uint32_t dest_g = 0;
          uint32_t dest_r = 0;
          for (int j = pPixelWeights->src_start_; j <= pPixelWeights->src_end_;
               j++) {
            uint32_t pixel_weight =
                pPixelWeights->weights_[j - pPixelWeights->src_start_];
            const uint8_t* src_pixel = src_scan + j * src_bytes_per_pixel;
            FX_RGB_STRUCT<uint8_t> src_rgb =
                AdobeCmykToStandardRgb(255 - src_pixel[0], 255 - src_pixel[1],
                                       255 - src_pixel[2], 255 - src_pixel[3]);
            dest_b += pixel_weight * src_rgb.blue;
            dest_g += pixel_weight * src_rgb.green;
            dest_r += pixel_weight * src_rgb.red;
          }
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_b);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_g);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_r);
          dest_scan += dest_bytes_per_pixel - 3;
          break;
        });
      }
      case TransformMethod::kArgbToArgb: {
        UNSAFE_TODO({
          uint32_t dest_alpha = 0;
          uint32_t dest_r = 0;
          uint32_t dest_g = 0;
          uint32_t dest_b = 0;
          for (int j = pPixelWeights->src_start_; j <= pPixelWeights->src_end_;
               j++) {
            uint32_t pixel_weight =
                pPixelWeights->weights_[j - pPixelWeights->src_start_];
            const uint8_t* src_pixel = src_scan + j * src_bytes_per_pixel;
            pixel_weight = pixel_weight * src_pixel[3] / 255;
            dest_b += pixel_weight * (*src_pixel++);
            dest_g += pixel_weight * (*src_pixel++);
            dest_r += pixel_weight * (*src_pixel);
            dest_alpha += pixel_weight;
          }
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_b);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_g);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_r);
          *dest_scan++ = CStretchEngine::PixelFromFixed(dest_alpha * 255);
          break;
        });
      }
    }
  }
}

void ProgressiveDecoder::Resample(const RetainPtr<CFX_DIBitmap>& pDeviceBitmap,
                                  int32_t src_line,
                                  uint8_t* src_scan,
                                  Format src_format) {
  if (src_line < 0 || src_line >= src_height_) {
    return;
  }

  ResampleScanline(pDeviceBitmap, src_line, decode_buf_, src_format);
}

FXDIB_Format ProgressiveDecoder::GetBitmapFormat() const {
  switch (image_type_) {
    case FXCODEC_IMAGE_JPG:
#ifdef PDF_ENABLE_XFA_BMP
    case FXCODEC_IMAGE_BMP:
#endif  // PDF_ENABLE_XFA_BMP
      return GetBitsPerPixel() <= 24 ? FXDIB_Format::kBgr : FXDIB_Format::kBgrx;
#ifdef PDF_ENABLE_XFA_PNG
    case FXCODEC_IMAGE_PNG:
#endif  // PDF_ENABLE_XFA_PNG
#ifdef PDF_ENABLE_XFA_TIFF
    case FXCODEC_IMAGE_TIFF:
#endif  // PDF_ENABLE_XFA_TIFF
    default:
      // TODO(crbug.com/355630556): Consider adding support for
      // `FXDIB_Format::kBgraPremul`
      return FXDIB_Format::kBgra;
  }
}

std::pair<FXCODEC_STATUS, size_t> ProgressiveDecoder::GetFrames() {
  if (!(status_ == FXCODEC_STATUS::kFrameReady ||
        status_ == FXCODEC_STATUS::kFrameToBeContinued)) {
    return {FXCODEC_STATUS::kError, 0};
  }

  switch (image_type_) {
#ifdef PDF_ENABLE_XFA_BMP
    case FXCODEC_IMAGE_BMP:
#endif  // PDF_ENABLE_XFA_BMP
    case FXCODEC_IMAGE_JPG:
#ifdef PDF_ENABLE_XFA_PNG
    case FXCODEC_IMAGE_PNG:
#endif  // PDF_ENABLE_XFA_PNG
#ifdef PDF_ENABLE_XFA_TIFF
    case FXCODEC_IMAGE_TIFF:
#endif  // PDF_ENABLE_XFA_TIFF
      frame_number_ = 1;
      status_ = FXCODEC_STATUS::kDecodeReady;
      return {status_, 1};
#ifdef PDF_ENABLE_XFA_GIF
    case FXCODEC_IMAGE_GIF: {
      auto* ctx = static_cast<CFX_GifContext*>(context_.get());
      while (true) {
        ProgressiveDecoderContext::Status readResult = ctx->GetFrame();
        frame_number_ =
            readResult == ProgressiveDecoderContext::Status::kSuccess
                ? ctx->GetFrameNum()
                : 0;
        while (readResult == ProgressiveDecoderContext::Status::kContinue) {
          FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
          if (!GifReadMoreData(&error_status)) {
            return {error_status, 0};
          }

          readResult = ctx->GetFrame();
          frame_number_ =
              readResult == ProgressiveDecoderContext::Status::kSuccess
                  ? ctx->GetFrameNum()
                  : 0;
        }
        if (readResult == ProgressiveDecoderContext::Status::kSuccess) {
          status_ = FXCODEC_STATUS::kDecodeReady;
          return {status_, frame_number_};
        }
        context_ = nullptr;
        status_ = FXCODEC_STATUS::kError;
        return {status_, 0};
      }
    }
#endif  // PDF_ENABLE_XFA_GIF
    default:
      return {FXCODEC_STATUS::kError, 0};
  }
}

FXCODEC_STATUS ProgressiveDecoder::StartDecode(RetainPtr<CFX_DIBitmap> bitmap) {
  CHECK(bitmap);
  CHECK_EQ(bitmap->GetWidth(), src_width_);
  CHECK_EQ(bitmap->GetHeight(), src_height_);
  CHECK_GT(src_width_, 0);
  CHECK_GT(src_height_, 0);

  const FXDIB_Format format = bitmap->GetFormat();
  CHECK(format == FXDIB_Format::kBgra || format == FXDIB_Format::kBgr ||
        format == FXDIB_Format::kBgrx);

  if (status_ != FXCODEC_STATUS::kDecodeReady) {
    return FXCODEC_STATUS::kError;
  }

  if (frame_number_ == 0) {
    return FXCODEC_STATUS::kError;
  }

  if (bitmap->GetWidth() > 65535 || bitmap->GetHeight() > 65535) {
    return FXCODEC_STATUS::kError;
  }

  frame_cur_ = 0;
  device_bitmap_ = std::move(bitmap);
  switch (image_type_) {
#ifdef PDF_ENABLE_XFA_BMP
    case FXCODEC_IMAGE_BMP:
      return BmpStartDecode();
#endif  // PDF_ENABLE_XFA_BMP
#ifdef PDF_ENABLE_XFA_GIF
    case FXCODEC_IMAGE_GIF:
      return GifStartDecode();
#endif  // PDF_ENABLE_XFA_GIF
    case FXCODEC_IMAGE_JPG:
      return JpegStartDecode();
#ifdef PDF_ENABLE_XFA_PNG
    case FXCODEC_IMAGE_PNG:
      return PngStartDecode();
#endif  // PDF_ENABLE_XFA_PNG
#ifdef PDF_ENABLE_XFA_TIFF
    case FXCODEC_IMAGE_TIFF:
      status_ = FXCODEC_STATUS::kDecodeToBeContinued;
      return status_;
#endif  // PDF_ENABLE_XFA_TIFF
    default:
      return FXCODEC_STATUS::kError;
  }
}

FXCODEC_STATUS ProgressiveDecoder::ContinueDecode() {
  if (status_ != FXCODEC_STATUS::kDecodeToBeContinued) {
    return FXCODEC_STATUS::kError;
  }

  switch (image_type_) {
    case FXCODEC_IMAGE_JPG:
      return JpegContinueDecode();
#ifdef PDF_ENABLE_XFA_BMP
    case FXCODEC_IMAGE_BMP:
      return BmpContinueDecode();
#endif  // PDF_ENABLE_XFA_BMP
#ifdef PDF_ENABLE_XFA_GIF
    case FXCODEC_IMAGE_GIF:
      return GifContinueDecode();
#endif  // PDF_ENABLE_XFA_GIF
#ifdef PDF_ENABLE_XFA_PNG
    case FXCODEC_IMAGE_PNG:
      return PngContinueDecode();
#endif  // PDF_ENABLE_XFA_PNG
#ifdef PDF_ENABLE_XFA_TIFF
    case FXCODEC_IMAGE_TIFF:
      return TiffContinueDecode();
#endif  // PDF_ENABLE_XFA_TIFF
    default:
      return FXCODEC_STATUS::kError;
  }
}

int ProgressiveDecoder::GetScanlineSize() const {
  // Can't be called before basic image metadata is decoded.
  CHECK_NE(src_components_count_, 0);
  CHECK_NE(src_width_, 0);

  FX_SAFE_INT32 result = src_width_;
  result *= src_components_count_;

  return FxAlignToBoundary<4>(result).ValueOrDie();
}

}  // namespace fxcodec
