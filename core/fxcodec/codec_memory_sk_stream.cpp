// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcodec/codec_memory_sk_stream.h"

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"

CodecMemorySkStream::CodecMemorySkStream(
    RetainPtr<CFX_CodecMemory> codec_memory)
    : codec_memory_(std::move(codec_memory)) {}

CodecMemorySkStream::~CodecMemorySkStream() = default;

size_t CodecMemorySkStream::read(void* buffer, size_t size) {
  // SAFETY: Relying on the caller to pass correct `buffer` and `size`.
  uint8_t* bytes = static_cast<uint8_t*>(buffer);
  auto byte_span = UNSAFE_BUFFERS(pdfium::span(bytes, size));
  return codec_memory_->ReadBlock(byte_span);
}

bool CodecMemorySkStream::isAtEnd() const {
  return codec_memory_->IsEOF();
}
