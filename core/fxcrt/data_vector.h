// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_DATA_VECTOR_H_
#define CORE_FXCRT_DATA_VECTOR_H_

#include <ranges>
#include <type_traits>
#include <vector>

#include "core/fxcrt/fx_memory_wrappers.h"

namespace fxcrt {

template <typename T>
using DataVector = std::vector<T, FxAllocAllocator<T>>;

template <typename T = void, typename Range>
auto ToDataVector(Range&& range) {
  using ValueType =
      std::conditional_t<std::is_void_v<T>,
                         std::decay_t<std::ranges::range_value_t<Range>>, T>;
  return DataVector<ValueType>(std::ranges::begin(range),
                               std::ranges::end(range));
}

}  // namespace fxcrt

using fxcrt::DataVector;
using fxcrt::ToDataVector;

#endif  // CORE_FXCRT_DATA_VECTOR_H_
