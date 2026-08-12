// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/skia_scanline_decoder.h"

#include <memory>
#include <optional>
#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/span_util.h"
#include "core/fxcrt/zip.h"
#include "core/fxge/dib/fx_dib.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/codec/SkJpegDecoder.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkStream.h"

namespace fxcodec {

namespace {

pdfium::span<const uint8_t> JpegScanSOI(pdfium::span<const uint8_t> src_span) {
  if (src_span.size() < 2) {
    return src_span;
  }
  for (size_t offset = 0; offset + 1 < src_span.size(); ++offset) {
    if (src_span[offset] == 0xff && src_span[offset + 1] == 0xd8) {
      return src_span.subspan(offset);
    }
  }
  return src_span;
}

uint32_t ScaledJpegSize(uint32_t dim, uint32_t scale_denom) {
  return (dim + scale_denom - 1) / scale_denom;
}

}  // namespace

// static
std::unique_ptr<ScanlineDecoder> SkiaScanlineDecoder::Create(
    pdfium::span<const uint8_t> src_span,
    uint32_t width,
    uint32_t height,
    int num_components,
    bool color_transform,
    uint32_t scale_denom) {
  auto decoder = std::make_unique<SkiaScanlineDecoder>();
  if (!decoder->CreateImpl(src_span, width, height, num_components,
                           color_transform, scale_denom)) {
    return nullptr;
  }
  return decoder;
}

SkiaScanlineDecoder::SkiaScanlineDecoder() = default;

SkiaScanlineDecoder::~SkiaScanlineDecoder() = default;

bool SkiaScanlineDecoder::CreateImpl(pdfium::span<const uint8_t> src_span,
                                     uint32_t width,
                                     uint32_t height,
                                     int num_components,
                                     bool color_transform,
                                     uint32_t scale_denom) {
  DCHECK(scale_denom == 1 || scale_denom == 2 || scale_denom == 4 ||
         scale_denom == 8);
  if (!color_transform) {
    return false;
  }
  if (num_components != 1 && num_components != 3) {
    return false;
  }

  scale_denom_ = scale_denom;
  src_span = JpegScanSOI(src_span);
  if (src_span.size() < 2) {
    return false;
  }

  auto stream = SkMemoryStream::MakeDirect(src_span.data(), src_span.size());
  SkCodec::Result result = SkCodec::kSuccess;
  decoder_ = SkJpegDecoder::Decode(std::move(stream), &result);
  if (!decoder_ || result != SkCodec::kSuccess) {
    return false;
  }

  orig_width_ = decoder_->dimensions().width();
  orig_height_ = decoder_->dimensions().height();
  if (orig_width_ < static_cast<int>(width) ||
      orig_height_ < static_cast<int>(height)) {
    return false;
  }

  comps_ = num_components;
  bpc_ = 8;
  output_width_ = ScaledJpegSize(orig_width_, scale_denom_);
  output_height_ = ScaledJpegSize(orig_height_, scale_denom_);

  CalcPitch();
  scanline_buf_ = DataVector<uint8_t>(pitch_);

  if (comps_ == 3) {
    FX_SAFE_SIZE_T safe_row_size = output_width_;
    safe_row_size *= 4;
    row_decode_buf_.resize(safe_row_size.ValueOrDie());
  }

  return true;
}

bool SkiaScanlineDecoder::Rewind() {
  if (!decoder_) {
    return false;
  }

  SkColorType color_type = kUnknown_SkColorType;
  switch (comps_) {
    case 1:
      color_type = kGray_8_SkColorType;
      break;
    case 3:
      color_type = kRGB_888x_SkColorType;
      break;
    default:
      NOTREACHED();
  }

  SkImageInfo dst_info = SkImageInfo::Make(output_width_, output_height_,
                                           color_type, kOpaque_SkAlphaType);
  SkCodec::Options options;
  return decoder_->startScanlineDecode(dst_info, &options) == SkCodec::kSuccess;
}

pdfium::span<uint8_t> SkiaScanlineDecoder::GetNextLine() {
  if (!decoder_) {
    return pdfium::span<uint8_t>();
  }

  if (comps_ == 3) {
    int lines = decoder_->getScanlines(row_decode_buf_.data(), 1,
                                       row_decode_buf_.size());
    if (lines <= 0) {
      return pdfium::span<uint8_t>();
    }
    pdfium::span<const uint8_t> src_bytes(row_decode_buf_);
    pdfium::span<uint8_t> dst_bytes(scanline_buf_);
    auto src_span =
        fxcrt::reinterpret_span<const FX_RGBA_STRUCT<uint8_t>>(src_bytes).first(
            static_cast<size_t>(output_width_));
    auto dst_span = fxcrt::reinterpret_span<FX_RGB_STRUCT<uint8_t>>(dst_bytes);
    for (auto [src, dst] : fxcrt::Zip(src_span, dst_span)) {
      dst.red = src.red;
      dst.green = src.green;
      dst.blue = src.blue;
    }
    return scanline_buf_;
  }

  int lines = decoder_->getScanlines(scanline_buf_.data(), 1, pitch_);
  if (lines <= 0) {
    return pdfium::span<uint8_t>();
  }
  return scanline_buf_;
}

uint32_t SkiaScanlineDecoder::GetSrcOffset() {
  return 0;
}

void SkiaScanlineDecoder::CalcPitch() {
  DCHECK_GT(output_width_, 0);
  pitch_ = static_cast<uint32_t>(output_width_) * comps_;
  pitch_ += 3;
  pitch_ /= 4;
  pitch_ *= 4;
}

// static
std::optional<JpegModule::ImageInfo> SkiaScanlineDecoder::LoadInfo(
    pdfium::span<const uint8_t> src_span) {
  src_span = JpegScanSOI(src_span);
  if (src_span.size() < 2) {
    return std::nullopt;
  }

  auto stream = SkMemoryStream::MakeDirect(src_span.data(), src_span.size());
  SkCodec::Result result = SkCodec::kSuccess;
  auto decoder = SkJpegDecoder::Decode(std::move(stream), &result);
  if (!decoder || result != SkCodec::kSuccess) {
    return std::nullopt;
  }

  SkImageInfo sk_info = decoder->getInfo();
  JpegModule::ImageInfo info;
  info.width = sk_info.width();
  info.height = sk_info.height();
  switch (sk_info.colorType()) {
    case kGray_8_SkColorType:
      info.num_components = 1;
      break;
    case kRGB_565_SkColorType:
    case kRGB_888x_SkColorType:
    case kRGBA_8888_SkColorType:
    case kBGRA_8888_SkColorType:
      info.num_components = 3;
      break;
    default:
      info.num_components = 4;
      break;
  }
  info.color_transform = true;
  info.bits_per_components = 8;
  return info;
}

}  // namespace fxcodec
