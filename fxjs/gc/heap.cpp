// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/gc/heap.h"

#include <memory>
#include <utility>
#include <variant>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/unowned_ptr.h"
#include "v8/include/cppgc/heap.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-platform.h"

namespace {

size_t g_platform_ref_count = 0;
v8::Platform* g_platform = nullptr;
v8::Isolate* g_isolate = nullptr;

}  // namespace

// Taken from v8/samples/cppgc/cppgc-for-v8-embedders.cc.
// Adaptper that makes the global v8::Platform compatible with a
// cppgc::Platform.
class CFXGC_Platform final : public cppgc::Platform {
 public:
  explicit CFXGC_Platform(v8::Isolate* isolate) : isolate_(isolate) {}
  ~CFXGC_Platform() override = default;

  cppgc::PageAllocator* GetPageAllocator() override {
    return g_platform->GetPageAllocator();
  }

  double MonotonicallyIncreasingTime() override {
    return g_platform->MonotonicallyIncreasingTime();
  }

  std::shared_ptr<cppgc::TaskRunner> GetForegroundTaskRunner() override {
    // V8's default platform creates a new task runner when passed the
    // v8::Isolate pointer the first time. For non-default platforms this will
    // require getting the appropriate task runner.
    return g_platform->GetForegroundTaskRunner(isolate_ ? isolate_.get()
                                                        : g_isolate);
  }

  std::shared_ptr<cppgc::TaskRunner> GetForegroundTaskRunner(
      cppgc::TaskPriority priority) override {
    // V8's default platform creates a new task runner when passed the
    // v8::Isolate pointer the first time. For non-default platforms this will
    // require getting the appropriate task runner.
    return g_platform->GetForegroundTaskRunner(
        isolate_ ? isolate_.get() : g_isolate, priority);
  }

  std::unique_ptr<cppgc::JobHandle> PostJob(
      cppgc::TaskPriority priority,
      std::unique_ptr<cppgc::JobTask> job_task) override {
    return g_platform->PostJob(priority, std::move(job_task));
  }

 private:
  UnownedPtr<v8::Isolate> const isolate_;
};

void FXGC_Initialize(v8::Platform* platform, v8::Isolate* isolate) {
  if (platform) {
    DCHECK(!g_platform);
    g_platform = platform;
    g_isolate = isolate;
  }
}

void FXGC_Release() {
  if (g_platform && g_platform_ref_count == 0) {
    g_platform = nullptr;
    g_isolate = nullptr;
  }
}

FXGC_Heap::FXGC_Heap(std::unique_ptr<cppgc::Heap> heap)
    : heap_(std::move(heap)) {}

FXGC_Heap::FXGC_Heap(v8::CppHeap* attached_heap) : heap_(attached_heap) {}

FXGC_Heap::~FXGC_Heap() {
  if (cppgc::Heap* heap = GetStandaloneHeap()) {
    DCHECK_GT(g_platform_ref_count, 0u);
    --g_platform_ref_count;
    FXGC_ForceGarbageCollection(heap);
  }
}

cppgc::AllocationHandle& FXGC_Heap::GetAllocationHandle() {
  return std::visit(
      [](const auto& heap) -> cppgc::AllocationHandle& {
        return heap->GetAllocationHandle();
      },
      heap_);
}

cppgc::HeapHandle& FXGC_Heap::GetHeapHandle() {
  return std::visit(
      [](const auto& heap) -> cppgc::HeapHandle& {
        return heap->GetHeapHandle();
      },
      heap_);
}

std::unique_ptr<v8::CppHeap> FXGC_CreateCppHeap(v8::Platform* platform) {
  v8::Platform* p = platform ? platform : g_platform;
  if (!p) {
    return nullptr;
  }
  v8::CppHeapCreateParams params({});
  params.marking_support = cppgc::Heap::MarkingType::kAtomic;
  params.sweeping_support =
      cppgc::Heap::SweepingType::kIncrementalAndConcurrent;
  return v8::CppHeap::Create(p, params);
}

FXGCScopedHeap FXGC_CreateHeap(v8::Isolate* isolate) {
  // If XFA is included at compile-time, but JS is disabled at run-time,
  // we may still attempt to build a CPDFXFA_Context which will want a
  // heap. But we can't make one because JS is disabled.
  // TODO(tsepez): Stop the context from even being created.
  if (!g_platform) {
    return nullptr;
  }

  if (isolate) {
    v8::CppHeap* cpp_heap = isolate->GetCppHeap();
    if (cpp_heap) {
      return std::make_unique<FXGC_Heap>(cpp_heap);
    }
  }

  ++g_platform_ref_count;
  auto heap = cppgc::Heap::Create(
      std::make_shared<CFXGC_Platform>(isolate),
      cppgc::Heap::HeapOptions{
          {},
          cppgc::Heap::StackSupport::kNoConservativeStackScan,
          cppgc::Heap::MarkingType::kAtomic,
          cppgc::Heap::SweepingType::kIncrementalAndConcurrent,
          {}});
  return std::make_unique<FXGC_Heap>(std::move(heap));
}

void FXGC_ForceGarbageCollection(FXGC_Heap* heap) {
  if (!heap) {
    return;
  }
  if (heap->GetAttachedHeap()) {
    FXGC_ForceGarbageCollection(heap->GetAttachedHeap());
    return;
  }
  if (heap->GetStandaloneHeap()) {
    FXGC_ForceGarbageCollection(heap->GetStandaloneHeap());
  }
}

void FXGC_ForceGarbageCollection(cppgc::Heap* heap) {
  if (heap) {
    heap->ForceGarbageCollectionSlow("FXGC", "ForceGarbageCollection",
                                     cppgc::Heap::StackState::kNoHeapPointers);
  }
}

void FXGC_ForceGarbageCollection(v8::CppHeap* cpp_heap) {
  if (cpp_heap) {
    cpp_heap->CollectGarbageForTesting(
        cppgc::EmbedderStackState::kNoHeapPointers);
  }
}
