// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_CONTAINERS_TO_VECTOR_H_
#define CORE_FXCRT_CONTAINERS_TO_VECTOR_H_

#include <ranges>
#include <type_traits>
#include <vector>

namespace pdfium {

template <typename T = void, typename Range>
auto ToVector(Range&& range) {
  using ValueType =
      std::conditional_t<std::is_void_v<T>,
                         std::decay_t<std::ranges::range_value_t<Range>>, T>;
  return std::vector<ValueType>(std::ranges::begin(range),
                                std::ranges::end(range));
}

}  // namespace pdfium

#endif  // CORE_FXCRT_CONTAINERS_TO_VECTOR_H_
