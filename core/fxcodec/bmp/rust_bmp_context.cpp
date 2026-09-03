// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/bmp/rust_bmp_context.h"

#include <utility>
#include <vector>

#include "core/fxcodec/bmp/rust_bmp_ffi.rs.h"
#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/byteorder.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {

namespace {

constexpr size_t kMinHeaderBytes = 18;
constexpr size_t kHeaderSizeOffset = 14;
constexpr size_t kMaxUncompressedBytes = 1024 * 1024 * 1024;  // 1 GiB

// Malformed BMP files may specify a data offset that is before the end of the
// header. Clamp the offset to ensure it points past the header.
void ClampBmpHeaderOffset(pdfium::span<uint8_t> header, size_t total_size) {
  if (header.size() >= kMinHeaderBytes && header[0] == 'B' &&
      header[1] == 'M') {
    uint32_t bi_size =
        fxcrt::GetUInt32LSBFirst(header.subspan<kHeaderSizeOffset, 4>());
    FX_SAFE_UINT32 safe_min_off = bi_size;
    safe_min_off += kHeaderSizeOffset;
    uint32_t off_bits = fxcrt::GetUInt32LSBFirst(header.subspan<10, 4>());
    if (safe_min_off.IsValid()) {
      uint32_t min_off = safe_min_off.ValueOrDie();
      if (off_bits < min_off && min_off <= total_size) {
        fxcrt::PutUInt32LSBFirst(min_off, header.subspan<10, 4>());
      }
    }
  }
}

}  // namespace

RustBmpContext::RustBmpContext(ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {}

RustBmpContext::~RustBmpContext() = default;

void RustBmpContext::SetCodecMemory(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = std::move(codec_memory);
}

FX_FILESIZE RustBmpContext::GetAvailInput() const {
  if (!codec_memory_) {
    return 0;
  }
  return codec_memory_->GetSize();
}

void RustBmpContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  SetCodecMemory(std::move(codec_memory));
}

ProgressiveDecoderContext::Status RustBmpContext::DecodeImage() {
  return ContinueDecode();
}

ProgressiveDecoderContext::Status RustBmpContext::ReadHeader(
    int32_t* width,
    int32_t* height,
    int32_t* components,
    pdfium::span<const FX_ARGB>* palette,
    CFX_DIBAttribute* attribute) {
  if (header_read_) {
    *width = static_cast<int32_t>(width_);
    *height = static_cast<int32_t>(height_);
    *components = components_;
    *palette = {};
    if (attribute) {
      attribute->dpi_unit_ = CFX_DIBAttribute::kResUnitMeter;
      attribute->x_dpi_ = dpi_x_;
      attribute->y_dpi_ = dpi_y_;
    }
    return ProgressiveDecoderContext::Status::kSuccess;
  }

  const auto span = codec_memory_->GetBufferSpan();
  if (span.size() < kMinHeaderBytes) {
    return ProgressiveDecoderContext::Status::kContinue;
  }

  ClampBmpHeaderOffset(codec_memory_->GetBufferSpan(), span.size());

  rust_bmp::BmpHeaderInfo header_info{};
  rust::Slice<const uint8_t> src_slice(span);
  rust_bmp::DecodeStatus status =
      rust_bmp::read_bmp_info(src_slice, header_info);
  if (status == rust_bmp::DecodeStatus::Continue) {
    return ProgressiveDecoderContext::Status::kContinue;
  }
  if (status == rust_bmp::DecodeStatus::Error) {
    return ProgressiveDecoderContext::Status::kError;
  }

  FX_SAFE_SIZE_T safe_uncompressed = header_info.width;
  safe_uncompressed *= header_info.height;
  safe_uncompressed *= header_info.components;
  if (!safe_uncompressed.IsValid() ||
      safe_uncompressed.ValueOrDie() > kMaxUncompressedBytes) {
    return ProgressiveDecoderContext::Status::kError;
  }

  width_ = header_info.width;
  height_ = header_info.height;
  components_ = header_info.components;

  if (span.size() >= 46) {
    uint32_t bi_size =
        fxcrt::GetUInt32LSBFirst(span.subspan<kHeaderSizeOffset, 4>());
    if (bi_size >= 40) {
      dpi_x_ =
          static_cast<int32_t>(fxcrt::GetUInt32LSBFirst(span.subspan<38, 4>()));
      dpi_y_ =
          static_cast<int32_t>(fxcrt::GetUInt32LSBFirst(span.subspan<42, 4>()));
    }
  }

  *width = static_cast<int32_t>(width_);
  *height = static_cast<int32_t>(height_);
  *components = components_;
  *palette = {};
  if (attribute) {
    attribute->dpi_unit_ = CFX_DIBAttribute::kResUnitMeter;
    attribute->x_dpi_ = dpi_x_;
    attribute->y_dpi_ = dpi_y_;
  }
  header_read_ = true;
  return ProgressiveDecoderContext::Status::kSuccess;
}

ProgressiveDecoderContext::Status RustBmpContext::StartDecode() {
  ProgressiveDecoderContextDelegate::Format format =
      (components_ == 4) ? ProgressiveDecoderContextDelegate::Format::kBgra
                         : ProgressiveDecoderContextDelegate::Format::kBgr;
  if (!delegate_->PrepareDirectOutput(width_, height_, format)) {
    return ProgressiveDecoderContext::Status::kError;
  }

  return ContinueDecode();
}

ProgressiveDecoderContext::Status RustBmpContext::ContinueDecode() {
  pdfium::span<const uint8_t> src_span = codec_memory_->GetBufferSpan();
  rust::Slice<const uint8_t> src_slice(src_span);

  FX_SAFE_SIZE_T safe_row_bytes = width_;
  safe_row_bytes *= components_;
  safe_row_bytes += 3;
  safe_row_bytes /= 4;
  safe_row_bytes *= 4;
  if (!safe_row_bytes.IsValid()) {
    return ProgressiveDecoderContext::Status::kError;
  }
  size_t row_bytes = safe_row_bytes.ValueOrDie();

  FX_SAFE_SIZE_T buf_size = row_bytes;
  buf_size *= height_;
  if (!buf_size.IsValid()) {
    return ProgressiveDecoderContext::Status::kError;
  }

  std::vector<uint8_t> decoded_buf(buf_size.ValueOrDie());
  rust::Slice<uint8_t> out_slice(decoded_buf);
  rust_bmp::DecodeStatus status =
      rust_bmp::decode_bmp_to_buf(src_slice, out_slice, row_bytes);
  if (status == rust_bmp::DecodeStatus::Continue) {
    return ProgressiveDecoderContext::Status::kContinue;
  }
  if (status == rust_bmp::DecodeStatus::Error) {
    return ProgressiveDecoderContext::Status::kError;
  }

  pdfium::span<const uint8_t> buf_span(decoded_buf);
  for (size_t y = 0; y < height_; ++y) {
    pdfium::span<const uint8_t> row_span =
        buf_span.subspan(y * row_bytes, row_bytes);
    delegate_->ResampleScanline(static_cast<int>(y), row_span);
  }

  return ProgressiveDecoderContext::Status::kSuccess;
}

}  // namespace fxcodec
