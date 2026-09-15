// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/fuzzers/xfa_process_state.h"

#include "fxjs/gc/heap.h"
#include "v8/include/libplatform/libplatform.h"

// Pass nullptr to always get a standalone cppgc heap, even though an isolate
// is available. Since V8 attaches a default CppHeap to every isolate, passing
// `isolate` would instead hand back that attached heap, and collecting it
// performs a full V8 GC. That requires both --expose-gc and the isolate to
// have been entered, neither of which holds when ForceGCAndPump() runs from a
// fuzzer case, nor when the destructor runs from a static destructor at exit.
XFAProcessState::XFAProcessState(v8::Platform* platform, v8::Isolate* isolate)
    : platform_(platform), isolate_(isolate), heap_(FXGC_CreateHeap(nullptr)) {}

XFAProcessState::~XFAProcessState() {
  FXGC_ForceGarbageCollection(heap_.get());
}

void XFAProcessState::ForceGCAndPump() {
  FXGC_ForceGarbageCollection(heap_.get());
  while (v8::platform::PumpMessageLoop(platform_, isolate_)) {
    continue;
  }
}
