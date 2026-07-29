// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/bmp/skia_bmp_decoder.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "core/fxcodec/bmp/bmp_decoder_delegate.h"
#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/codec_memory_sk_stream.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder_context.h"
#include "core/fxcrt/byteorder.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/calculate_pitch.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "third_party/skia/experimental/rust_bmp/decoder/SkBmpRustDecoder.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/core/SkAlphaType.h"
#include "third_party/skia/include/core/SkColorType.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkStream.h"

namespace fxcodec {

namespace {

constexpr size_t kMinHeaderBytes = 18;
constexpr size_t kHeaderSizeOffset = 14;

// Malformed BMP files may specify a data offset that is before the end of the
// header. Clamp the offset to ensure it points past the header.
void ClampBmpHeaderOffset(pdfium::span<uint8_t, kMinHeaderBytes> header,
                          size_t total_size) {
  if (header[0] == 'B' && header[1] == 'M') {
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

class SkiaBmpContext final : public ProgressiveDecoderContext {
 public:
  explicit SkiaBmpContext(BmpDecoderDelegate* delegate) : delegate_(delegate) {}
  ~SkiaBmpContext() override = default;

  void SetCodecMemory(RetainPtr<CFX_CodecMemory> codec_memory) {
    codec_memory_ = std::move(codec_memory);
  }

  // ProgressiveDecoderContext:
  FX_FILESIZE GetAvailInput() const override {
    if (!codec_memory_) {
      return 0;
    }
    return codec_memory_->GetSize();
  }

  void Input(RetainPtr<CFX_CodecMemory> codec_memory) override {
    SetCodecMemory(std::move(codec_memory));
  }

  ProgressiveDecoderContext::Status ReadHeader(
      int32_t* width,
      int32_t* height,
      bool* tb_flag,
      int32_t* components,
      pdfium::span<const FX_ARGB>* palette,
      CFX_DIBAttribute* pAttribute) {
    if (header_read_) {
      SkImageInfo info = decoder_->getInfo();
      *width = info.width();
      *height = info.height();
      *tb_flag = top_down_;
      *components = components_;
      *palette = {};
      return ProgressiveDecoderContext::Status::kSuccess;
    }

    const auto span = codec_memory_->GetBufferSpan();
    if (span.size() < kMinHeaderBytes) {
      return ProgressiveDecoderContext::Status::kContinue;
    }

    // There are enough bytes to construct and initialize a decoder,
    // if not already created by a previous call.
    if (!decoder_) {
      // Clamp malformed BMP data offsets prior to decoding.
      ClampBmpHeaderOffset(
          codec_memory_->GetBufferSpan().first<kMinHeaderBytes>(), span.size());
      codec_memory_->Seek(0);
      auto stream = std::make_unique<CodecMemorySkStream>(codec_memory_);
      SkCodec::Result result = SkCodec::kInvalidInput;
      decoder_ = SkBmpRustDecoder::Decode(std::move(stream), &result);
      if (result == SkCodec::kIncompleteInput) {
        codec_memory_->Seek(0);
        decoder_.reset();
        return ProgressiveDecoderContext::Status::kContinue;
      }
      if (result != SkCodec::kSuccess || !decoder_) {
        decoder_.reset();
        return ProgressiveDecoderContext::Status::kError;
      }
    }

    SkImageInfo info = decoder_->getInfo();
    *width = info.width();
    *height = info.height();

    // Extract metadata from the DIB header.
    uint32_t bpp = 24;
    uint32_t bi_size =
        fxcrt::GetUInt32LSBFirst(span.subspan<kHeaderSizeOffset, 4>());
    uint32_t bi_compression = 0;
    uint32_t bi_clr_used = 0;
    if (bi_size == 12) {
      if (span.size() < 26) {
        return ProgressiveDecoderContext::Status::kContinue;
      }
      bpp = fxcrt::GetUInt16LSBFirst(span.subspan<24, 2>());
    } else if (bi_size >= 40) {
      if (span.size() < 50) {
        return ProgressiveDecoderContext::Status::kContinue;
      }
      top_down_ = static_cast<int32_t>(
                      fxcrt::GetUInt32LSBFirst(span.subspan<22, 4>())) < 0;
      bpp = fxcrt::GetUInt16LSBFirst(span.subspan<28, 2>());
      bi_compression = fxcrt::GetUInt32LSBFirst(span.subspan<30, 4>());
      bi_clr_used = fxcrt::GetUInt32LSBFirst(span.subspan<46, 4>());
    } else {
      return ProgressiveDecoderContext::Status::kError;
    }

    if (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 16 && bpp != 24 &&
        bpp != 32) {
      return ProgressiveDecoderContext::Status::kError;
    }
    if (bpp <= 8 && bi_clr_used > (1u << bpp)) {
      return ProgressiveDecoderContext::Status::kError;
    }
    if (bi_compression > 6) {
      return ProgressiveDecoderContext::Status::kError;
    }

    bpp_ = bpp;
    bi_size_ = bi_size;
    bi_compression_ = bi_compression;
    bi_clr_used_ = bi_clr_used;

    *tb_flag = top_down_;
    components_ = (bpp == 32) ? 4 : 3;
    *components = components_;
    *palette = {};
    if (pAttribute) {
      pAttribute->dpi_unit_ = CFX_DIBAttribute::kResUnitMeter;
      pAttribute->x_dpi_ = 0;
      pAttribute->y_dpi_ = 0;
    }
    header_read_ = true;
    return ProgressiveDecoderContext::Status::kSuccess;
  }

  ProgressiveDecoderContext::Status DecodeImage(size_t frame_index) override {
    if (!header_read_ || !decoder_) {
      return ProgressiveDecoderContext::Status::kError;
    }
    if (!ValidatePaletteIndices()) {
      return ProgressiveDecoderContext::Status::kError;
    }

    ProgressiveDecoderContext::Status status = StartIncrementalDecode();
    if (status != ProgressiveDecoderContext::Status::kSuccess) {
      return status;
    }

    int rows_decoded = 0;
    // Decode available scanlines into the internal buffer.
    SkCodec::Result result = decoder_->incrementalDecode(&rows_decoded);

    // Forward newly decoded scanlines to the delegate.
    if (rows_decoded > rows_forwarded_) {
      int width = decoder_->getInfo().width();
      int height = decoder_->getInfo().height();
      std::optional<uint32_t> maybe_src_row_bytes =
          fxge::CalculatePitch32(32, width);
      std::optional<uint32_t> maybe_dst_row_bytes =
          fxge::CalculatePitch32(components_ == 4 ? 32 : 24, width);
      if (!maybe_src_row_bytes.has_value() ||
          !maybe_dst_row_bytes.has_value()) {
        return ProgressiveDecoderContext::Status::kError;
      }
      size_t src_row_bytes = maybe_src_row_bytes.value();
      size_t dst_row_bytes = maybe_dst_row_bytes.value();
      std::vector<uint8_t> row_buffer;
      if (components_ != 4) {
        row_buffer.resize(dst_row_bytes);
      }

      int start_row = top_down_ ? rows_forwarded_ : (height - rows_decoded);
      int end_row = top_down_ ? rows_decoded : (height - rows_forwarded_);
      if (start_row < 0) {
        return ProgressiveDecoderContext::Status::kError;
      }
      FX_SAFE_SIZE_T safe_src_row_end = static_cast<size_t>(start_row);
      safe_src_row_end *= src_row_bytes;
      for (int r = start_row; r < end_row; ++r) {
        size_t src_row_start = safe_src_row_end.ValueOrDie();
        safe_src_row_end += src_row_bytes;
        if (!safe_src_row_end.IsValid() ||
            safe_src_row_end.ValueOrDie() > decoded_image_buf_.size()) {
          return ProgressiveDecoderContext::Status::kError;
        }
        auto src_row = pdfium::span(decoded_image_buf_)
                           .subspan(src_row_start, src_row_bytes);
        uint32_t dest_row = static_cast<uint32_t>(r);
        if (components_ == 4) {
          delegate_->BmpReadScanline(dest_row, src_row);
        } else {
          Forward3ComponentRow(dest_row, src_row, row_buffer, width);
        }
      }
      rows_forwarded_ = rows_decoded;
    }

    if (result == SkCodec::kSuccess) {
      return ProgressiveDecoderContext::Status::kSuccess;
    }
    if (result == SkCodec::kIncompleteInput) {
      return ProgressiveDecoderContext::Status::kContinue;
    }
    return ProgressiveDecoderContext::Status::kError;
  }

 private:
  bool ValidatePaletteIndices() {
    if (bpp_ > 8 || bi_compression_ != 0 || bi_clr_used_ == 0) {
      return true;
    }
    // Reject palette index references that exceed the palette size.
    uint32_t num_colors = bi_clr_used_;
    if (num_colors >= (1u << bpp_)) {
      return true;
    }
    FX_SAFE_SIZE_T safe_off = bi_size_;
    safe_off += kHeaderSizeOffset;
    safe_off += num_colors * 4;
    if (!safe_off.IsValid()) {
      return false;
    }
    size_t off = safe_off.ValueOrDie();
    const auto buf = codec_memory_->GetBufferSpan();
    if (buf.size() >= kHeaderSizeOffset) {
      uint32_t off_bits = fxcrt::GetUInt32LSBFirst(buf.subspan<10, 4>());
      if (off_bits >= off && off_bits <= buf.size()) {
        off = off_bits;
      }
    }
    int width = decoder_->getInfo().width();
    int height = decoder_->getInfo().height();
    std::optional<uint32_t> maybe_row_stride =
        fxge::CalculatePitch32(bpp_, width);
    if (!maybe_row_stride.has_value()) {
      return false;
    }
    FX_SAFE_SIZE_T safe_row_off = off;
    size_t row_stride = maybe_row_stride.value();
    for (int y = 0; y < height; ++y, safe_row_off += row_stride) {
      if (!safe_row_off.IsValid()) {
        return false;
      }
      size_t row_off = safe_row_off.ValueOrDie();
      if (row_off >= buf.size()) {
        break;
      }
      auto row_span = buf.subspan(row_off);
      if (bpp_ == 8) {
        size_t valid_bytes =
            std::min(static_cast<size_t>(width), row_span.size());
        for (size_t x = 0; x < valid_bytes; ++x) {
          if (row_span[x] >= num_colors) {
            return false;
          }
        }
      } else if (bpp_ == 4) {
        for (int x = 0; x < width; ++x) {
          size_t byte_idx = static_cast<size_t>(x) / 2;
          if (byte_idx >= row_span.size()) {
            break;
          }
          uint8_t val = (x % 2 == 0) ? (row_span[byte_idx] >> 4)
                                     : (row_span[byte_idx] & 0x0f);
          if (val >= num_colors) {
            return false;
          }
        }
      } else {
        CHECK_EQ(bpp_, 1u);
        for (int x = 0; x < width; ++x) {
          size_t byte_idx = static_cast<size_t>(x) / 8;
          if (byte_idx >= row_span.size()) {
            break;
          }
          uint8_t val = (row_span[byte_idx] >> (7 - (x % 8))) & 1;
          if (val >= num_colors) {
            return false;
          }
        }
      }
    }
    return true;
  }

  ProgressiveDecoderContext::Status StartIncrementalDecode() {
    // Initialize incremental decoding into the destination buffer.
    if (decode_started_) {
      return ProgressiveDecoderContext::Status::kSuccess;
    }
    SkImageInfo dst_info = decoder_->getInfo()
                               .makeColorType(kBGRA_8888_SkColorType)
                               .makeAlphaType(decoder_->getInfo().alphaType());
    size_t row_bytes = dst_info.minRowBytes();
    FX_SAFE_SIZE_T safe_buf_size = dst_info.height();
    safe_buf_size *= row_bytes;
    if (!safe_buf_size.IsValid()) {
      return ProgressiveDecoderContext::Status::kError;
    }
    decoded_image_buf_.resize(safe_buf_size.ValueOrDie());
    SkCodec::Result result = decoder_->startIncrementalDecode(
        dst_info, decoded_image_buf_.data(), row_bytes);
    if (result == SkCodec::kIncompleteInput) {
      return ProgressiveDecoderContext::Status::kContinue;
    }
    if (result != SkCodec::kSuccess) {
      return ProgressiveDecoderContext::Status::kError;
    }
    decode_started_ = true;
    return ProgressiveDecoderContext::Status::kSuccess;
  }

  void Forward3ComponentRow(uint32_t dest_row,
                            pdfium::span<const uint8_t> src_row,
                            pdfium::span<uint8_t> row_buffer,
                            int width) {
    CHECK_EQ(components_, 3);
    for (int x = 0; x < width; ++x) {
      row_buffer[x * 3 + 0] = src_row[x * 4 + 0];
      row_buffer[x * 3 + 1] = src_row[x * 4 + 1];
      row_buffer[x * 3 + 2] = src_row[x * 4 + 2];
    }
    delegate_->BmpReadScanline(dest_row, row_buffer);
  }

  UnownedPtr<BmpDecoderDelegate> const delegate_;
  std::unique_ptr<SkCodec> decoder_;
  RetainPtr<CFX_CodecMemory> codec_memory_;
  std::vector<uint8_t> decoded_image_buf_;
  int rows_forwarded_ = 0;
  int components_ = 3;
  uint32_t bpp_ = 24;
  uint32_t bi_size_ = 40;
  uint32_t bi_compression_ = 0;
  uint32_t bi_clr_used_ = 0;
  bool top_down_ = false;
  bool header_read_ = false;
  bool decode_started_ = false;
};

}  // namespace

// static
std::unique_ptr<ProgressiveDecoderContext> SkiaBmpDecoder::StartDecode(
    BmpDecoderDelegate* pDelegate) {
  return std::make_unique<SkiaBmpContext>(pDelegate);
}

// static
ProgressiveDecoderContext::Status SkiaBmpDecoder::ReadHeader(
    ProgressiveDecoderContext* context,
    int32_t* width,
    int32_t* height,
    bool* tb_flag,
    int32_t* components,
    pdfium::span<const FX_ARGB>* palette,
    CFX_DIBAttribute* pAttribute) {
  auto* ctx = static_cast<SkiaBmpContext*>(context);
  return ctx->ReadHeader(width, height, tb_flag, components, palette,
                         pAttribute);
}

}  // namespace fxcodec
