// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/embedder_test_constants.h"

#include "build/build_config.h"
#include "core/fxge/cfx_defaultrenderdevice.h"

namespace pdfium {

const char* AnnotationStampWithApChecksum() {
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
#if BUILDFLAG(IS_WIN)
    return "10425299f19e223eda9a3930ef4bed3c";
#elif BUILDFLAG(IS_APPLE)
    return "71c46babfe48a5a0dff03ad3edeb2918";
#else
    return "50f53c0a869ad33a8dd2b580740a63c8";
#endif
  }
#if BUILDFLAG(IS_APPLE)
  return "587311ad93447614cbe5887df14caa78";
#else
  return "2908fd6166f795dfd73c607ec12c5356";
#endif
}

const char* Bug890322Checksum() {
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
    return "793689536cf64fe792c2f241888c0cf3";
  }
  return "6c674642154408e877d88c6c082d67e9";
}

const char* HelloWorldRemovedChecksum() {
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
#if BUILDFLAG(IS_WIN)
    return "7db00f520f0838da10ef45373af3f6aa";
#elif BUILDFLAG(IS_APPLE)
    return "99cefacd84710f3fb2e3d129ba68ae8a";
#else
    return "6e0307348e7c1b92f2f061f92f62fd45";
#endif
  }
#if BUILDFLAG(IS_APPLE)
  return "6e1cae48a2e35c521dee4ca502f48af6";
#else
  return "4a9b80f675f7f3bf2da1b02f12449e4b";
#endif
}

const char* ManyRectanglesChecksum() {
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
    return "4e7e280c1597222afcb0ee3bb90ec119";
  }
  return "b0170c575b65ecb93ebafada0ff0f038";
}

const char* RectanglesChecksum() {
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
    return "b4e411a6b5ffa59a50efede2efece597";
  }
  return "0a90de37f52127619c3dfb642b5fa2fe";
}

const char* TextFormChecksum() {
  if (CFX_DefaultRenderDevice::UseSkiaRenderer()) {
#if BUILDFLAG(IS_WIN)
    return "e83f49ddea0822734a16b986e9935732";
#elif BUILDFLAG(IS_APPLE)
    return "32913f21b1012b74eef37737a03a92b7";
#else
    return "b259776fd156003e2a594d1c7ce2d8d7";
#endif
  }
#if BUILDFLAG(IS_APPLE)
  return "fa2bf756942a950101fc147fc4ef3f82";
#else
  return "6f86fe1dbed5965d91aec6e0b829e29f";
#endif
}

}  // namespace pdfium
