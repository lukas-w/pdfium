// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FXJS_GC_HEAP_H_
#define FXJS_GC_HEAP_H_

#include <memory>
#include <variant>

#include "core/fxcrt/unowned_ptr_exclusion.h"
#include "v8/include/cppgc/allocation.h"

namespace cppgc {
class Heap;
class HeapHandle;
}  // namespace cppgc

namespace v8 {
class CppHeap;
class Isolate;
class Platform;
}  // namespace v8

// Abstracts between two different kinds of garbage-collected heaps:
// those associated with V8 (v8::CppHeap) and standalone (cppgc::Heap).
class FXGC_Heap {
 public:
  explicit FXGC_Heap(std::unique_ptr<cppgc::Heap> heap);
  explicit FXGC_Heap(v8::Isolate* isolate);
  FXGC_Heap(const FXGC_Heap&) = delete;
  FXGC_Heap& operator=(const FXGC_Heap&) = delete;
  ~FXGC_Heap();

  cppgc::AllocationHandle& GetAllocationHandle();
  cppgc::HeapHandle& GetHeapHandle();

  cppgc::Heap* GetStandaloneHeap() const {
    auto* standalone = std::get_if<std::unique_ptr<cppgc::Heap>>(&heap_);
    return standalone ? standalone->get() : nullptr;
  }
  v8::CppHeap* GetAttachedHeap() const {
    auto* attached = std::get_if<v8::CppHeap*>(&heap_);
    return attached ? *attached : nullptr;
  }
  v8::Isolate* GetIsolate() const { return isolate_; }

 private:
  // Non-owning pointers; outlived by this wrapper because GC objects finalized
  // during isolate/heap teardown hold UnownedPtrs back to this FXGC_Heap.
  std::variant<std::unique_ptr<cppgc::Heap>, v8::CppHeap*> const heap_;
  UNOWNED_PTR_EXCLUSION v8::Isolate* const isolate_ = nullptr;
};

using FXGCScopedHeap = std::unique_ptr<FXGC_Heap>;

void FXGC_Initialize(v8::Platform* platform, v8::Isolate* isolate);
void FXGC_Release();
FXGCScopedHeap FXGC_CreateHeap(v8::Isolate* isolate);
std::unique_ptr<v8::CppHeap> FXGC_CreateCppHeap(v8::Platform* platform);
void FXGC_ForceGarbageCollection(FXGC_Heap* heap);
void FXGC_ForceGarbageCollection(cppgc::Heap* heap);
void FXGC_ForceGarbageCollection(v8::CppHeap* cpp_heap);

#define CONSTRUCT_VIA_MAKE_GARBAGE_COLLECTED \
  template <typename T>                      \
  friend class cppgc::MakeGarbageCollectedTrait

#endif  // FXJS_GC_HEAP_H_
