// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_CONTAINERS_EXTEND_H_
#define CORE_FXCRT_CONTAINERS_EXTEND_H_

#include <algorithm>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

// This is Chromium's base/containers/extend.h, adapted to work with PDFium's
// codebase with the following modifications:
//
// - Updated include guards.
// - Replaced namespace base with namespace pdfium.
// - Added template parameter `Alloc` to support `DataVector`.

namespace pdfium {

// Append to `dst` all elements of `src` by std::move-ing them out of `src`.
// After this operation, `src` will be empty.
template <typename T, typename Alloc>
void Extend(std::vector<T, Alloc>& dst, std::vector<T, Alloc>&& src) {
  dst.insert(dst.end(), std::make_move_iterator(src.begin()),
             std::make_move_iterator(src.end()));
  src.clear();
}

// Appends `range` to `dst`, copying them out of `range`.
template <typename T,
          typename Alloc,
          typename Range,
          typename Proj = std::identity>
  requires std::ranges::range<Range> &&
           std::indirectly_unary_invocable<Proj, std::ranges::iterator_t<Range>>
void Extend(std::vector<T, Alloc>& dst, Range&& range, Proj proj = {}) {
  if constexpr (std::ranges::sized_range<Range>) {
    dst.reserve(dst.size() + std::ranges::size(range));
  }
  std::ranges::transform(std::forward<Range>(range), std::back_inserter(dst),
                         std::move(proj));
}

}  // namespace pdfium

#endif  // CORE_FXCRT_CONTAINERS_EXTEND_H_
