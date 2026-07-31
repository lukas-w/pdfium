// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/libpng_png_context.h"

#ifdef USE_SYSTEM_LIBPNG
#include <png.h>
#else
#include "third_party/libpng/png.h"
#endif

extern "C" {
int _png_set_read_and_error_fns(png_structrp png_ptr,
                                void* user_ctx,
                                char* error_buf);
}

namespace fxcodec {

LibpngPngContext::LibpngPngContext(PngDecoderDelegate* pDelegate)
    : delegate_(pDelegate) {
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

}  // namespace fxcodec
