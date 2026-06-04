// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/compiler_specific.h"
#include "testing/fuzzers/xfa_codec_fuzzer.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: required from fuzzer API.
  return UNSAFE_BUFFERS(XFACodecFuzzer::Fuzz(data, size, FXCODEC_IMAGE_BMP));
}
