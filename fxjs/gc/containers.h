// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FXJS_GC_CONTAINERS_H_
#define FXJS_GC_CONTAINERS_H_

#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <vector>

#include "fxjs/gc/container_trace.h"
#include "v8/include/cppgc/garbage-collected.h"
#include "v8/include/cppgc/member.h"
#include "v8/include/cppgc/type-traits.h"
#include "v8/include/cppgc/visitor.h"

// These container wrappers are intended to replace stack-local containers
// (e.g. `std::vector`, `std::set`, `std::map`) of garbage-collected objects.
// On the stack, instances should generally be held via `cppgc::Persistent<>`.
// Although holding `cppgc::Persistent<>` handles on the stack is typically an
// anti-pattern in garbage-collected environments, it is required here due to
// the "malloc'd" backing-stores. Holding a single persistent handle
// to a wrapper minimizes overhead compared to holding individual handles for
// every element.

namespace fxgc {
namespace internal {

template <typename T>
using MemberIfNeeded =
    std::conditional_t<cppgc::IsGarbageCollectedOrMixinTypeV<T>,
                       cppgc::Member<T>,
                       T>;

}  // namespace internal

template <typename T>
class GCedVector final : public cppgc::GarbageCollected<GCedVector<T>> {
 public:
  template <typename V = cppgc::Visitor>
  void Trace(V* visitor) const {
    ContainerTrace(visitor, storage);
  }

  std::vector<cppgc::Member<T>> storage;
};

template <typename T>
class GCedSet final : public cppgc::GarbageCollected<GCedSet<T>> {
 public:
  template <typename V = cppgc::Visitor>
  void Trace(V* visitor) const {
    ContainerTrace(visitor, storage);
  }

  std::set<cppgc::Member<T>> storage;
};

template <typename T>
class GCedList final : public cppgc::GarbageCollected<GCedList<T>> {
 public:
  template <typename V = cppgc::Visitor>
  void Trace(V* visitor) const {
    ContainerTrace(visitor, storage);
  }

  std::list<cppgc::Member<T>> storage;
};

template <typename Key, typename Value>
class GCedMap final : public cppgc::GarbageCollected<GCedMap<Key, Value>> {
 public:
  static_assert(cppgc::IsGarbageCollectedOrMixinTypeV<Key> ||
                    cppgc::IsGarbageCollectedOrMixinTypeV<Value>,
                "Use std::map instead for non-gc'd key-value pairs");

  template <typename V = cppgc::Visitor>
  void Trace(V* visitor) const {
    ContainerTrace(visitor, storage);
  }

  std::map<internal::MemberIfNeeded<Key>, internal::MemberIfNeeded<Value>>
      storage;
};

}  // namespace fxgc

using fxgc::GCedList;
using fxgc::GCedMap;
using fxgc::GCedSet;
using fxgc::GCedVector;

#endif  // FXJS_GC_CONTAINERS_H_
