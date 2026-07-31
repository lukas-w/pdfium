// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/libjpeg_jpeg_context.h"

#include <stdint.h>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/fx_dib.h"

namespace fxcodec {

namespace {

void JpegLoadAttribute(const jpeg_decompress_struct& info,
                       CFX_DIBAttribute* pAttribute) {
  pAttribute->x_dpi_ = info.X_density;
  pAttribute->y_dpi_ = info.Y_density;
  pAttribute->dpi_unit_ =
      static_cast<CFX_DIBAttribute::ResUnit>(info.density_unit);
}

}  // namespace

LibjpegJpegContext::LibjpegJpegContext() {
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
  jpeg_destroy_decompress(&common_.cinfo);
}

FX_FILESIZE LibjpegJpegContext::GetAvailInput() const {
  return static_cast<FX_FILESIZE>(common_.source_mgr.bytes_in_buffer);
}

void LibjpegJpegContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
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

int LibjpegJpegContext::ReadHeader(int* width,
                                   int* height,
                                   int* nComps,
                                   CFX_DIBAttribute* pAttribute) {
  DCHECK(pAttribute);

  int ret = jpeg_common_read_header(&common_, TRUE);
  if (ret == -1) {
    return kFatal;
  }
  if (ret == JPEG_SUSPENDED) {
    return kNeedsMoreInput;
  }
  if (ret != JPEG_HEADER_OK) {
    return kError;
  }
  *width = common_.cinfo.image_width;
  *height = common_.cinfo.image_height;
  *nComps = common_.cinfo.num_components;
  JpegLoadAttribute(common_.cinfo, pAttribute);
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
