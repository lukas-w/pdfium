// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/rust_jpeg_scanline_decoder.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

#include "core/fxcodec/jpeg/rust_jpeg_ffi.rs.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/span_util.h"

namespace fxcodec {

namespace {

constexpr size_t kKnownBadHeaderWithInvalidHeightByteOffsetStarts[] = {94, 163};
constexpr size_t kSofMarkerByteOffset = 5;

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

bool IsSofSegment(pdfium::span<const uint8_t> src_span, size_t marker_offset) {
  if (src_span.size() <= marker_offset + 1) {
    return false;
  }
  const auto header_marker = src_span.subspan(marker_offset);
  return header_marker[0] == 0xff && header_marker[1] >= 0xc0 &&
         header_marker[1] <= 0xcf;
}

bool HasKnownBadHeaderWithInvalidHeight(pdfium::span<const uint8_t> src_span,
                                        size_t dimension_offset,
                                        uint32_t orig_width) {
  if (src_span.size() <= dimension_offset + 3u) {
    return false;
  }
  if (dimension_offset < kSofMarkerByteOffset) {
    return false;
  }
  if (!IsSofSegment(src_span, dimension_offset - kSofMarkerByteOffset)) {
    return false;
  }
  const auto header_dimensions = src_span.subspan(dimension_offset);
  uint8_t expected_width_byte1 = (orig_width >> 8) & 0xff;
  uint8_t expected_width_byte2 = orig_width & 0xff;
  return header_dimensions[0] == 0xff && header_dimensions[1] == 0xff &&
         header_dimensions[2] == expected_width_byte1 &&
         header_dimensions[3] == expected_width_byte2;
}

void PatchUpKnownBadHeaderWithInvalidHeight(pdfium::span<uint8_t> src_span,
                                            size_t dimension_offset,
                                            uint32_t orig_height) {
  auto data = src_span.subspan(dimension_offset);
  data[0] = (orig_height >> 8) & 0xff;
  data[1] = orig_height & 0xff;
}

void PatchUpTrailer(pdfium::span<uint8_t> src_span) {
  src_span[src_span.size() - 2] = 0xff;
  src_span[src_span.size() - 1] = 0xd9;
}

}  // namespace

// static
std::unique_ptr<ScanlineDecoder> RustJpegScanlineDecoder::Create(
    pdfium::span<const uint8_t> src_span,
    uint32_t width,
    uint32_t height,
    int num_components,
    bool color_transform,
    uint32_t scale_denom) {
  auto decoder = std::make_unique<RustJpegScanlineDecoder>();
  if (!decoder->CreateImpl(src_span, width, height, num_components,
                           color_transform, scale_denom)) {
    return nullptr;
  }
  return decoder;
}

RustJpegScanlineDecoder::RustJpegScanlineDecoder() = default;

RustJpegScanlineDecoder::~RustJpegScanlineDecoder() = default;

bool RustJpegScanlineDecoder::CreateImpl(pdfium::span<const uint8_t> src_span,
                                         uint32_t width,
                                         uint32_t height,
                                         int num_components,
                                         bool color_transform,
                                         uint32_t scale_denom) {
  src_span = JpegScanSOI(src_span);
  if (src_span.size() < 2) {
    return false;
  }

  // SAFETY: const_cast<> doesn't change size.
  pdfium::span<uint8_t> writable_src = UNSAFE_BUFFERS(
      pdfium::span(const_cast<uint8_t*>(src_span.data()), src_span.size()));
  PatchUpTrailer(writable_src);

  rust_jpeg::JpegHeaderInfo header_info{};
  rust::Slice<const uint8_t> src_slice(src_span);
  if (!rust_jpeg::read_jpeg_info(src_slice, header_info)) {
    bool patched = false;
    for (size_t offset : kKnownBadHeaderWithInvalidHeightByteOffsetStarts) {
      if (HasKnownBadHeaderWithInvalidHeight(src_span, offset, width)) {
        PatchUpKnownBadHeaderWithInvalidHeight(writable_src, offset, height);
        patched = true;
        break;
      }
    }
    if (!patched || !rust_jpeg::read_jpeg_info(src_slice, header_info)) {
      return false;
    }
  }

  orig_width_ = header_info.width;
  orig_height_ = header_info.height;

  DCHECK(scale_denom == 1 || scale_denom == 2 || scale_denom == 4 ||
         scale_denom == 8);

  comps_ = header_info.num_components;
  bpc_ = header_info.bits_per_component;

  constexpr int kDctSize = 8;
  const int max_h_samp = std::max<int>(1, header_info.max_h_samp);
  const int max_v_samp = std::max<int>(1, header_info.max_v_samp);
  if (orig_width_ % (max_h_samp * kDctSize) != 0 ||
      orig_height_ % (max_v_samp * kDctSize) != 0) {
    scale_denom = 1;
  }

  output_width_ = ScaledJpegSize(orig_width_, scale_denom);
  output_height_ = ScaledJpegSize(orig_height_, scale_denom);

  CalcPitch();

  FX_SAFE_SIZE_T buf_size = pitch_;
  buf_size *= output_height_;
  if (!buf_size.IsValid()) {
    return false;
  }
  decoded_image_ = DataVector<uint8_t>(buf_size.ValueOrDie());

  rust::Slice<uint8_t> out_slice(decoded_image_);
  if (!rust_jpeg::decode_jpeg_to_buf(src_slice, out_slice, pitch_,
                                     scale_denom)) {
    return false;
  }

  next_row_ = 0;
  return true;
}

bool RustJpegScanlineDecoder::Rewind() {
  next_row_ = 0;
  return true;
}

pdfium::span<uint8_t> RustJpegScanlineDecoder::GetNextLine() {
  if (next_row_ >= static_cast<uint32_t>(output_height_)) {
    return pdfium::span<uint8_t>();
  }

  size_t offset = next_row_ * pitch_;
  if (offset + pitch_ > decoded_image_.size()) {
    return pdfium::span<uint8_t>();
  }

  ++next_row_;
  return pdfium::span(decoded_image_).subspan(offset, pitch_);
}

uint32_t RustJpegScanlineDecoder::GetSrcOffset() {
  return 0;
}

void RustJpegScanlineDecoder::CalcPitch() {
  DCHECK_GT(output_width_, 0);
  pitch_ = static_cast<uint32_t>(output_width_) * comps_;
  pitch_ += 3;
  pitch_ /= 4;
  pitch_ *= 4;
}

// static
std::optional<JpegModule::ImageInfo> RustJpegScanlineDecoder::LoadInfo(
    pdfium::span<const uint8_t> src_span) {
  src_span = JpegScanSOI(src_span);
  if (src_span.size() < 2) {
    return std::nullopt;
  }

  rust_jpeg::JpegHeaderInfo header_info{};
  rust::Slice<const uint8_t> src_slice(src_span);
  if (!rust_jpeg::read_jpeg_info(src_slice, header_info)) {
    return std::nullopt;
  }

  JpegModule::ImageInfo info;
  info.width = header_info.width;
  info.height = header_info.height;
  info.num_components = header_info.num_components;
  info.color_transform = header_info.color_transform;
  info.bits_per_components = header_info.bits_per_component;
  return info;
}

}  // namespace fxcodec
