// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/png/cpngcontext.h"

#ifdef USE_SYSTEM_LIBPNG
#include <png.h>
#else
#include "third_party/libpng/png.h"
#endif

namespace fxcodec {

CPngContext::CPngContext(PngDecoderDelegate* pDelegate)
    : delegate_(pDelegate) {}

CPngContext::~CPngContext() {
  png_destroy_read_struct(png_ ? &png_ : nullptr, info_ ? &info_ : nullptr,
                          nullptr);
}

}  // namespace fxcodec
