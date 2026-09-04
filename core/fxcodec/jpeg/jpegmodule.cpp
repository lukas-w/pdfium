// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxcodec/jpeg/jpegmodule.h"

#include <memory>
#include <optional>

#include "build/build_config.h"
#include "core/fxcodec/jpeg/libjpeg_scanline_decoder.h"
#include "core/fxcodec/scanlinedecoder.h"
#include "core/fxcrt/span.h"

#if defined(PDF_ENABLE_RUST_JPEG)
#include "core/fxcodec/jpeg/rust_jpeg_scanline_decoder.h"
#endif

#if BUILDFLAG(IS_WIN)
#include <type_traits>

#include "core/fxcodec/jpeg/jpeg_common.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_memory.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/fx_dib.h"
#endif  // BUILDFLAG(IS_WIN)

namespace fxcodec {

// static
std::unique_ptr<ScanlineDecoder> JpegModule::CreateDecoder(
    pdfium::span<const uint8_t> src_span,
    uint32_t width,
    uint32_t height,
    int nComps,
    bool ColorTransform,
    uint32_t scale_denom) {
#if defined(PDF_ENABLE_RUST_JPEG)
  return RustJpegScanlineDecoder::Create(src_span, width, height, nComps,
                                         ColorTransform, scale_denom);
#else
  return LibjpegScanlineDecoder::Create(src_span, width, height, nComps,
                                        ColorTransform, scale_denom);
#endif
}

// static
std::optional<JpegModule::ImageInfo> JpegModule::LoadInfo(
    pdfium::span<const uint8_t> src_span) {
#if defined(PDF_ENABLE_RUST_JPEG)
  return RustJpegScanlineDecoder::LoadInfo(src_span);
#else
  return LibjpegScanlineDecoder::LoadInfo(src_span);
#endif
}

#if BUILDFLAG(IS_WIN)
bool JpegModule::JpegEncode(const RetainPtr<const CFX_DIBBase>& pSource,
                            uint8_t** dest_buf,
                            size_t* dest_size) {
  jpeg_error_mgr jerr;
  jerr.error_exit = jpeg_common_error_do_nothing;
  jerr.emit_message = jpeg_common_error_do_nothing_int;
  jerr.output_message = jpeg_common_error_do_nothing;
  jerr.format_message = jpeg_common_error_do_nothing_char;
  jerr.reset_error_mgr = jpeg_common_error_do_nothing;

  jpeg_compress_struct cinfo = {};  // Aggregate initialization.
  static_assert(std::is_aggregate_v<decltype(cinfo)>);
  cinfo.err = &jerr;
  jpeg_create_compress(&cinfo);
  const int bytes_per_pixel = pSource->GetBPP() / 8;
  uint32_t nComponents = bytes_per_pixel >= 3 ? 3 : 1;
  uint32_t pitch = pSource->GetPitch();
  uint32_t width = pdfium::checked_cast<uint32_t>(pSource->GetWidth());
  uint32_t height = pdfium::checked_cast<uint32_t>(pSource->GetHeight());
  FX_SAFE_UINT32 safe_buf_len = width;
  safe_buf_len *= height;
  safe_buf_len *= nComponents;
  safe_buf_len += 1024;
  if (!safe_buf_len.IsValid()) {
    return false;
  }

  uint32_t dest_buf_length = safe_buf_len.ValueOrDie();
  *dest_buf = FX_TryAlloc(uint8_t, dest_buf_length);
  const int MIN_TRY_BUF_LEN = 1024;
  while (!(*dest_buf) && dest_buf_length > MIN_TRY_BUF_LEN) {
    dest_buf_length >>= 1;
    *dest_buf = FX_TryAlloc(uint8_t, dest_buf_length);
  }
  if (!(*dest_buf)) {
    return false;
  }

  jpeg_destination_mgr dest;
  dest.init_destination = jpeg_common_dest_do_nothing;
  dest.term_destination = jpeg_common_dest_do_nothing;
  dest.empty_output_buffer = jpeg_common_dest_empty;
  dest.next_output_byte = *dest_buf;
  dest.free_in_buffer = dest_buf_length;
  cinfo.dest = &dest;
  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = nComponents;
  if (nComponents == 1) {
    cinfo.in_color_space = JCS_GRAYSCALE;
  } else if (nComponents == 3) {
    cinfo.in_color_space = JCS_RGB;
  } else {
    cinfo.in_color_space = JCS_CMYK;
  }
  uint8_t* line_buf = nullptr;
  if (nComponents > 1) {
    line_buf = FX_Alloc2D(uint8_t, width, nComponents);
  }

  jpeg_set_defaults(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);
  JSAMPROW row_pointer[1];
  JDIMENSION row;
  while (cinfo.next_scanline < cinfo.image_height) {
    pdfium::span<const uint8_t> src_scan =
        pSource->GetScanline(cinfo.next_scanline);
    if (nComponents > 1) {
      uint8_t* dest_scan = line_buf;
      if (nComponents == 3) {
        UNSAFE_TODO({
          for (uint32_t i = 0; i < width; i++) {
            ReverseCopy3Bytes(dest_scan, src_scan.data());
            dest_scan += 3;
            src_scan = src_scan.subspan(static_cast<size_t>(bytes_per_pixel));
          }
        });
      } else {
        UNSAFE_TODO({
          for (uint32_t i = 0; i < pitch; i++) {
            *dest_scan++ = ~src_scan.front();
            src_scan = src_scan.subspan<1u>();
          }
        });
      }
      row_pointer[0] = line_buf;
    } else {
      row_pointer[0] = const_cast<uint8_t*>(src_scan.data());
    }
    row = cinfo.next_scanline;
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
    UNSAFE_TODO({
      if (cinfo.next_scanline == row) {
        static constexpr size_t kJpegBlockSize = 1048576;
        *dest_buf =
            FX_Realloc(uint8_t, *dest_buf, dest_buf_length + kJpegBlockSize);
        dest.next_output_byte =
            *dest_buf + dest_buf_length - dest.free_in_buffer;
        dest_buf_length += kJpegBlockSize;
        dest.free_in_buffer += kJpegBlockSize;
      }
    });
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  FX_Free(line_buf);
  *dest_size = dest_buf_length - static_cast<size_t>(dest.free_in_buffer);

  return true;
}
#endif  // BUILDFLAG(IS_WIN)

}  // namespace fxcodec
