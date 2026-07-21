// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCODEC_CODEC_MEMORY_SK_STREAM_H_
#define CORE_FXCODEC_CODEC_MEMORY_SK_STREAM_H_

#include <stddef.h>

#include "core/fxcodec/cfx_codec_memory.h"
#include "core/fxcrt/retain_ptr.h"
#include "third_party/skia/include/core/SkStream.h"

// Implements/exposes `SkStream` API on top of `CFX_CodecMemory`.
//
// Note that the exposed API does *not* support seeking/rewinding, because this
// layer only has access to the `CFX_CodecMemory` pushed into `ContinueDecode`
// and can't reach for `IFX_SeekableReadStream` from which the bytes have been
// pulled into `CFX_CodecMemory`.  This lack of support for seeking/rewinding
// requires avoiding calling any Skia APIs that may necessitate this
// functionality later (e.g. calling `SkCodec::getFrameCount` may attempt to
// read metadata of subsequent frames).  This is not an issue today, because
// PDFium only needs to decode the first (or only) image / animation frame.
class CodecMemorySkStream final : public SkStream {
 public:
  explicit CodecMemorySkStream(RetainPtr<CFX_CodecMemory> codec_memory);
  ~CodecMemorySkStream() override;

  size_t read(void* buffer, size_t size) override;
  bool isAtEnd() const override;

 private:
  RetainPtr<CFX_CodecMemory> const codec_memory_;
};

#endif  // CORE_FXCODEC_CODEC_MEMORY_SK_STREAM_H_
