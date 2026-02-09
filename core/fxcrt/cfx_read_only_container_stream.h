// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_CFX_READ_ONLY_CONTAINER_STREAM_H_
#define CORE_FXCRT_CFX_READ_ONLY_CONTAINER_STREAM_H_

#include <stdint.h>

#include <utility>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"

template <typename Container>
concept HasSpanMethod = requires(const Container& t) {
  { t.span() };
};

template <typename Container>
class CFX_ReadOnlyContainerStream final : public CFX_ReadOnlySpanStream {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;

 private:
  explicit CFX_ReadOnlyContainerStream(Container data)
    requires(HasSpanMethod<Container>)
      : CFX_ReadOnlySpanStream(pdfium::as_bytes(data.span())),
        data_(std::move(data)) {}

  explicit CFX_ReadOnlyContainerStream(Container data)
    requires(!HasSpanMethod<Container>)
      : CFX_ReadOnlySpanStream(pdfium::as_byte_span(data)),
        data_(std::move(data)) {}

  ~CFX_ReadOnlyContainerStream() override {
    // Clear superclass's span before deleting container.
    span_ = {};
  }

  const Container data_;
};

extern template class CFX_ReadOnlyContainerStream<DataVector<uint8_t>>;
extern template class CFX_ReadOnlyContainerStream<FixedSizeDataVector<uint8_t>>;
extern template class CFX_ReadOnlyContainerStream<ByteString>;

using CFX_ReadOnlyDataVectorStream =
    CFX_ReadOnlyContainerStream<DataVector<uint8_t>>;

using CFX_ReadOnlyFixedSizeDataVectorStream =
    CFX_ReadOnlyContainerStream<FixedSizeDataVector<uint8_t>>;

using CFX_ReadOnlyByteStringStream = CFX_ReadOnlyContainerStream<ByteString>;

#endif  // CORE_FXCRT_CFX_READ_ONLY_CONTAINER_STREAM_H_
