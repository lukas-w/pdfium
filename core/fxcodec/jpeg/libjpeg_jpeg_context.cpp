// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/libjpeg_jpeg_context.h"

#include <stdint.h>
#include <utility>
#include <vector>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {
namespace {

void JpegLoadAttribute(const jpeg_decompress_struct& info,
                       CFX_DIBAttribute* attribute) {
  CHECK(attribute);
  attribute->x_dpi_ = info.X_density;
  attribute->y_dpi_ = info.Y_density;
  attribute->dpi_unit_ =
      static_cast<CFX_DIBAttribute::ResUnit>(info.density_unit);
}

void RGB2BGR(pdfium::span<uint8_t> buffer) {
  for (size_t i = 0; i + 2 < buffer.size(); i += 3) {
    std::swap(buffer[i], buffer[i + 2]);
  }
}

ProgressiveDecoderContextDelegate::Format GetCodecFormat(int num_components) {
  switch (num_components) {
    case 1:
      return ProgressiveDecoderContextDelegate::Format::k8bppGray;
    case 3:
      return ProgressiveDecoderContextDelegate::Format::kRgb;
    case 4:
      return ProgressiveDecoderContextDelegate::Format::kCmyk;
    default:
      return ProgressiveDecoderContextDelegate::Format::kInvalid;
  }
}

}  // namespace

LibjpegJpegContext::LibjpegJpegContext(
    ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {
  common_.cinfo.client_data = &common_;
  common_.cinfo.err = &common_.error_mgr;

  common_.error_mgr.error_exit = jpeg_common_error_fatal;
  common_.error_mgr.emit_message = jpeg_common_error_do_nothing_int;
  common_.error_mgr.output_message = jpeg_common_error_do_nothing;
  common_.error_mgr.format_message = jpeg_common_error_do_nothing_char;
  common_.error_mgr.reset_error_mgr = jpeg_common_error_do_nothing;

  common_.source_mgr.init_source = jpeg_common_src_do_nothing;
  common_.source_mgr.term_source = jpeg_common_src_do_nothing;
  common_.source_mgr.skip_input_data = jpeg_common_src_skip_data_or_record;
  common_.source_mgr.fill_input_buffer = jpeg_common_src_fill_buffer;
  common_.source_mgr.resync_to_restart = jpeg_common_src_resync;

  create_ok_ = jpeg_common_create_decompress(&common_);
  if (create_ok_) {
    common_.cinfo.src = &common_.source_mgr;
    common_.skip_size = 0;
  }
}

LibjpegJpegContext::~LibjpegJpegContext() {
  if (create_ok_) {
    jpeg_destroy_decompress(&common_.cinfo);
  }
}

FX_FILESIZE LibjpegJpegContext::GetAvailInput() const {
  return static_cast<FX_FILESIZE>(common_.source_mgr.bytes_in_buffer);
}

void LibjpegJpegContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = codec_memory;
  pdfium::span<uint8_t> src_buf = codec_memory->GetUnconsumedSpan();
  if (common_.skip_size) {
    if (common_.skip_size > src_buf.size()) {
      common_.source_mgr.bytes_in_buffer = 0;
      common_.skip_size -= src_buf.size();
      return;
    }
    src_buf = src_buf.subspan(common_.skip_size);
    common_.skip_size = 0;
  }
  common_.source_mgr.next_input_byte = src_buf.data();
  common_.source_mgr.bytes_in_buffer = src_buf.size();
}

void LibjpegJpegContext::SyncCodecMemory() {
  codec_memory_->Seek(codec_memory_->GetSize() -
                      common_.source_mgr.bytes_in_buffer);
}

FXCODEC_STATUS LibjpegJpegContext::StartDecode() {
  decode_row_ = 0;
  while (!StartScanline()) {
    FXCODEC_STATUS error_status = FXCODEC_STATUS::kError;
    SyncCodecMemory();
    if (!delegate_->ReadMoreData(std::nullopt, &error_status)) {
      return error_status;
    }
  }
  FX_SAFE_SIZE_T line_size = common_.cinfo.image_width;
  line_size *= common_.cinfo.num_components;
  line_buf_.resize(FxAlignToBoundary<4>(line_size).ValueOrDie());
  ProgressiveDecoderContextDelegate::Format src_format =
      GetCodecFormat(common_.cinfo.num_components);
  if (!delegate_->PrepareScanlineResampling(
          common_.cinfo.image_width, common_.cinfo.image_height, src_format)) {
    return FXCODEC_STATUS::kError;
  }
  return FXCODEC_STATUS::kDecodeToBeContinued;
}

FXCODEC_STATUS LibjpegJpegContext::ContinueDecode() {
  const bool is_rgb = common_.cinfo.num_components == 3;
  while (true) {
    int err_code = ReadScanline(line_buf_.data());
    if (err_code == LibjpegJpegContext::kFatal) {
      return FXCODEC_STATUS::kError;
    }
    if (err_code != LibjpegJpegContext::kOk) {
      FXCODEC_STATUS error_status = FXCODEC_STATUS::kDecodeFinished;
      SyncCodecMemory();
      if (delegate_->ReadMoreData(std::nullopt, &error_status)) {
        continue;
      }
      return error_status;
    }
    if (is_rgb) {
      RGB2BGR(pdfium::span(line_buf_).first(
          static_cast<size_t>(common_.cinfo.image_width) * 3));
    }
    delegate_->ResampleScanline(decode_row_++, line_buf_);
    if (decode_row_ >= static_cast<int>(common_.cinfo.image_height)) {
      return FXCODEC_STATUS::kDecodeFinished;
    }
  }
}

int LibjpegJpegContext::ReadHeader(int* width,
                                   int* height,
                                   int* nComps,
                                   CFX_DIBAttribute* attribute) {
  DCHECK(create_ok_);
  if (setjmp(common_.jmpbuf)) {
    return kFatal;
  }
  int ret = jpeg_read_header(&common_.cinfo, true);
  if (ret == JPEG_SUSPENDED) {
    return kNeedsMoreInput;
  }
  if (ret != JPEG_HEADER_OK) {
    return kError;
  }
  *width = common_.cinfo.image_width;
  *height = common_.cinfo.image_height;
  *nComps = common_.cinfo.num_components;
  JpegLoadAttribute(common_.cinfo, attribute);
  return kOk;
}

bool LibjpegJpegContext::StartScanline() {
  common_.cinfo.scale_denom = 1;
  return !!jpeg_common_start_decompress(&common_);
}

int LibjpegJpegContext::ReadScanline(uint8_t* dest_buf) {
  int nlines = jpeg_common_read_scanlines(&common_, &dest_buf, 1);
  if (nlines == -1) {
    return kFatal;
  }
  return nlines == 1 ? kOk : kError;
}

}  // namespace fxcodec
