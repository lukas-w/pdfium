// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/gc/containers.h"

#include <stdint.h>

#include "core/fxcrt/observed_ptr.h"
#include "fxjs/gc/heap.h"
#include "testing/fxgc_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/cppgc/garbage-collected.h"
#include "v8/include/cppgc/persistent.h"

namespace {

class ObservableGCedObject
    : public cppgc::GarbageCollected<ObservableGCedObject>,
      public Observable {
 public:
  CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED;

  void Trace(cppgc::Visitor* visitor) const {}

 private:
  ObservableGCedObject() = default;
};

}  // namespace

class ContainersUnitTest : public FXGCUnitTest {
 public:
  ContainersUnitTest() = default;
  ~ContainersUnitTest() override = default;

  ObservableGCedObject* CreateObject() {
    return cppgc::MakeGarbageCollected<ObservableGCedObject>(
        heap()->GetAllocationHandle());
  }
};

TEST_F(ContainersUnitTest, GCedVector) {
  cppgc::Persistent<GCedVector<ObservableGCedObject>> vec =
      cppgc::MakeGarbageCollected<GCedVector<ObservableGCedObject>>(
          heap()->GetAllocationHandle());

  ObservableGCedObject* obj = CreateObject();
  ObservedPtr<ObservableGCedObject> observer(obj);
  vec->storage.push_back(obj);

  FXGC_ForceGarbageCollection(heap());
  EXPECT_TRUE(observer);

  vec.Clear();
  FXGC_ForceGarbageCollection(heap());
  EXPECT_FALSE(observer);
}

TEST_F(ContainersUnitTest, GCedSet) {
  cppgc::Persistent<GCedSet<ObservableGCedObject>> set =
      cppgc::MakeGarbageCollected<GCedSet<ObservableGCedObject>>(
          heap()->GetAllocationHandle());

  ObservableGCedObject* obj = CreateObject();
  ObservedPtr<ObservableGCedObject> observer(obj);
  set->storage.insert(obj);

  FXGC_ForceGarbageCollection(heap());
  EXPECT_TRUE(observer);

  set.Clear();
  FXGC_ForceGarbageCollection(heap());
  EXPECT_FALSE(observer);
}

TEST_F(ContainersUnitTest, GCedList) {
  cppgc::Persistent<GCedList<ObservableGCedObject>> list =
      cppgc::MakeGarbageCollected<GCedList<ObservableGCedObject>>(
          heap()->GetAllocationHandle());

  ObservableGCedObject* obj = CreateObject();
  ObservedPtr<ObservableGCedObject> observer(obj);
  list->storage.push_back(obj);

  FXGC_ForceGarbageCollection(heap());
  EXPECT_TRUE(observer);

  list.Clear();
  FXGC_ForceGarbageCollection(heap());
  EXPECT_FALSE(observer);
}

TEST_F(ContainersUnitTest, GCedMap) {
  cppgc::Persistent<GCedMap<uint32_t, ObservableGCedObject>> map =
      cppgc::MakeGarbageCollected<GCedMap<uint32_t, ObservableGCedObject>>(
          heap()->GetAllocationHandle());

  ObservableGCedObject* obj = CreateObject();
  ObservedPtr<ObservableGCedObject> observer(obj);
  map->storage[123] = obj;

  FXGC_ForceGarbageCollection(heap());
  EXPECT_TRUE(observer);

  map.Clear();
  FXGC_ForceGarbageCollection(heap());
  EXPECT_FALSE(observer);
}
