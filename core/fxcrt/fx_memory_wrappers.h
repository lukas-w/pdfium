// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_FX_MEMORY_WRAPPERS_H_
#define CORE_FXCRT_FX_MEMORY_WRAPPERS_H_

#include <limits>
#include <type_traits>
#include <utility>

#include "core/fxcrt/fx_memory.h"

// Used with std::unique_ptr to FX_Free raw memory.
struct FxFreeDeleter {
  inline void operator()(void* ptr) const { FX_Free(ptr); }
};

// Escape hatch mechanism to allow non_arithmetic types into data partition.
template <typename T>
struct IsFXDataPartitionException : std::false_type {};

// Use ONLY for external / third-party types (e.g. Skia, Apple CoreGraphics)
// whose definitions cannot include `kIsDataPartitionSafe`.
#define FX_EXTERNAL_DATA_PARTITION_SAFE(T) \
  template <>                              \
  struct IsFXDataPartitionException<T> : std::true_type {}

template <typename T>
concept FxDataPartitionType = std::is_arithmetic_v<T> || std::is_enum_v<T> ||
                              IsFXDataPartitionException<T>::value ||
                              requires { requires T::kIsDataPartitionSafe; };

// Allocators for mapping STL containers onto Partition Alloc.
// Otherwise, replacing e.g. the FX_AllocUninit/FX_Free pairs with STL may
// undo some of the nice segregation that we get from PartitionAlloc.
template <class T, void* Alloc(size_t, size_t), void Free(void*)>
struct FxPartitionAllocAllocator {
  static_assert(FxDataPartitionType<T>,
                "Only numeric types allowed in this partition");

  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  template <class U>
  struct rebind {
    using other = FxPartitionAllocAllocator<U, Alloc, Free>;
  };

  FxPartitionAllocAllocator() noexcept = default;
  FxPartitionAllocAllocator(const FxPartitionAllocAllocator& other) noexcept =
      default;
  ~FxPartitionAllocAllocator() = default;

  template <typename U>
  FxPartitionAllocAllocator(
      const FxPartitionAllocAllocator<U, Alloc, Free>& other) noexcept {}

  pointer address(reference x) const noexcept { return &x; }
  const_pointer address(const_reference x) const noexcept { return &x; }
  pointer allocate(size_type n, const void* hint = 0) {
    return static_cast<pointer>(Alloc(n, sizeof(value_type)));
  }
  void deallocate(pointer p, size_type n) { Free(p); }
  size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max() / sizeof(value_type);
  }

  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    new (reinterpret_cast<void*>(p)) U(std::forward<Args>(args)...);
  }

  template <class U>
  void destroy(U* p) {
    p->~U();
  }

  // There's no state, so they are all the same,
  bool operator==(const FxPartitionAllocAllocator& that) { return true; }
  bool operator!=(const FxPartitionAllocAllocator& that) { return false; }
};

// Used to put backing store for std::vector<> and such into the
// general partition, ensuring they contain data only.
template <FxDataPartitionType T>
using FxAllocAllocator = FxPartitionAllocAllocator<T,
                                                   pdfium::internal::AllocOrDie,
                                                   pdfium::internal::Dealloc>;

// Used to put backing store for std::string<> and std::ostringstream<>
// into the string partition.
template <typename T>
using FxStringAllocator =
    FxPartitionAllocAllocator<T,
                              pdfium::internal::StringAllocOrDie,
                              pdfium::internal::StringDealloc>;

#endif  // CORE_FXCRT_FX_MEMORY_WRAPPERS_H_
