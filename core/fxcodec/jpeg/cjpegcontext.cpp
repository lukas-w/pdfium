// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/jpeg/cjpegcontext.h"

#include <stdint.h>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcrt/span.h"

namespace fxcodec {

CJpegContext::CJpegContext() {
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
}

CJpegContext::~CJpegContext() {
  jpeg_destroy_decompress(&common_.cinfo);
}

FX_FILESIZE CJpegContext::GetAvailInput() const {
  return static_cast<FX_FILESIZE>(common_.source_mgr.bytes_in_buffer);
}

void CJpegContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
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

}  // namespace fxcodec
