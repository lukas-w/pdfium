// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/libpng_png_context.h"

#include <setjmp.h>
#include <string.h>

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcodec/progressive_decoder_context_delegate.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"

#ifdef USE_SYSTEM_LIBPNG
#include <png.h>
#else
#include "third_party/libpng/png.h"
#endif

namespace {

constexpr double kPngGamma = 2.2;

}  // namespace

extern "C" {

void _png_error_data(png_structp png_ptr, png_const_charp error_msg) {
  if (png_get_error_ptr(png_ptr)) {
    UNSAFE_TODO(strncpy(static_cast<char*>(png_get_error_ptr(png_ptr)),
                        error_msg,
                        fxcodec::LibpngPngContext::kPngErrorSize - 1));
  }

  longjmp(png_jmpbuf(png_ptr), 1);
}

void _png_warning_data(png_structp png_ptr, png_const_charp error_msg) {}

void _png_get_header_func(png_structp png_ptr, png_infop info_ptr) {
  auto* context = reinterpret_cast<fxcodec::LibpngPngContext*>(
      png_get_progressive_ptr(png_ptr));
  if (!context) {
    return;
  }

  png_uint_32 width = 0;
  png_uint_32 height = 0;
  int bits_per_component = 0;
  int libpng_color_type = 0;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bits_per_component,
               &libpng_color_type, nullptr, nullptr, nullptr);
  if (bits_per_component > 8) {
    png_set_strip_16(png_ptr);
  } else if (bits_per_component < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  }

  if (libpng_color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }

  context->number_of_passes_ = png_set_interlace_handling(png_ptr);
  context->height_ = height;

  // Notifies the delegate of image dimensions and metadata.
  if (!context->delegate_->PrepareDirectOutput(
          width, height,
          fxcodec::ProgressiveDecoderContextDelegate::Format::kBgra)) {
    // Note that `png_error` function is marked as `PNG_NORETURN`.
    png_error(context->png_, "Read Header Callback Error");
  }
  int intent;
  if (png_get_sRGB(png_ptr, info_ptr, &intent)) {
    png_set_gamma(png_ptr, kPngGamma, 0.45455);
  } else {
    double image_gamma;
    if (png_get_gAMA(png_ptr, info_ptr, &image_gamma)) {
      png_set_gamma(png_ptr, kPngGamma, image_gamma);
    } else {
      png_set_gamma(png_ptr, kPngGamma, 0.45455);
    }
  }
  if (!(libpng_color_type & PNG_COLOR_MASK_COLOR)) {
    png_set_gray_to_rgb(png_ptr);
  }
  png_set_bgr(png_ptr);
  if (!(libpng_color_type & PNG_COLOR_MASK_ALPHA)) {
    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  }
  png_read_update_info(png_ptr, info_ptr);
}

void _png_get_end_func(png_structp png_ptr, png_infop info_ptr) {}

void _png_get_row_func(png_structp png_ptr,
                       png_bytep new_row,
                       png_uint_32 row_num,
                       int pass) {
  auto* context = reinterpret_cast<fxcodec::LibpngPngContext*>(
      png_get_progressive_ptr(png_ptr));
  if (!context || !context->bitmap_) {
    return;
  }

  pdfium::span<uint8_t> dst_buf = context->delegate_->AskScanlineBuf(row_num);
  CHECK(!dst_buf.empty());
  png_progressive_combine_row(png_ptr, dst_buf.data(), new_row);

  if ((pass == (context->number_of_passes_ - 1)) &&
      (row_num == (context->height_ - 1))) {
    context->finished_ = true;
  }
}

int _png_set_read_and_error_fns(png_structrp png_ptr,
                                void* user_ctx,
                                char* error_buf) {
  if (setjmp(png_jmpbuf(png_ptr))) {
    return 0;
  }
  png_set_progressive_read_fn(png_ptr, user_ctx, _png_get_header_func,
                              _png_get_row_func, _png_get_end_func);
  png_set_error_fn(png_ptr, error_buf, _png_error_data, _png_warning_data);
  return 1;
}

}  // extern "C"

namespace fxcodec {

LibpngPngContext::LibpngPngContext(ProgressiveDecoderContextDelegate* delegate)
    : delegate_(delegate) {
  png_ =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!png_) {
    return;
  }
  info_ = png_create_info_struct(png_);
  if (!info_) {
    return;
  }
  if (!_png_set_read_and_error_fns(png_, this, last_error_)) {
    png_destroy_read_struct(&png_, &info_, nullptr);
    png_ = nullptr;
    info_ = nullptr;
  }
}

LibpngPngContext::~LibpngPngContext() {
  png_destroy_read_struct(png_ ? &png_ : nullptr, info_ ? &info_ : nullptr,
                          nullptr);
}

void LibpngPngContext::Input(RetainPtr<CFX_CodecMemory> codec_memory) {
  codec_memory_ = std::move(codec_memory);
}

FXCODEC_STATUS LibpngPngContext::StartDecode(RetainPtr<CFX_DIBitmap> bitmap) {
  bitmap_ = std::move(bitmap);
  CHECK_EQ(bitmap_->GetFormat(), FXDIB_Format::kBgra);
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeToBeContinued;
  if (!delegate_->ReadMoreData(0, &status)) {
    return status;
  }
  return FXCODEC_STATUS::kDecodeToBeContinued;
}

FXCODEC_STATUS LibpngPngContext::ContinueDecode() {
  FXCODEC_STATUS status = FXCODEC_STATUS::kDecodeFinished;
  while (!finished_) {
    if (!ProcessData(codec_memory_)) {
      status = FXCODEC_STATUS::kError;
      break;
    }
    if (finished_) {
      break;
    }
    status = FXCODEC_STATUS::kError;
    if (!delegate_->ReadMoreData(std::nullopt, &status)) {
      break;
    }
  }
  bitmap_ = nullptr;
  return finished_ ? FXCODEC_STATUS::kDecodeFinished : status;
}

bool LibpngPngContext::ReadHeader(RetainPtr<CFX_CodecMemory> codec_memory) {
  return ProcessData(std::move(codec_memory));
}

bool LibpngPngContext::ProcessData(RetainPtr<CFX_CodecMemory> codec_memory) {
  if (setjmp(png_jmpbuf(png_))) {
    return false;
  }
  pdfium::span<uint8_t> src_buf = codec_memory->GetUnconsumedSpan();
  png_process_data(png_, info_, src_buf.data(), src_buf.size());

  // `libpng` always consumes all the data from `src_buf`, so
  // advance/seek `codec_memory` to the end of the buffer.
  codec_memory->Seek(codec_memory->GetSize());
  CHECK(codec_memory->GetUnconsumedSpan().empty());

  return true;
}

}  // namespace fxcodec
