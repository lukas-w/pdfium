// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_OBJECT_H_
#define FXJS_CJS_OBJECT_H_

#include "core/fxcrt/span.h"
#include "core/fxcrt/unowned_ptr.h"
#include "fxjs/cfxjs_engine.h"
#include "fxjs/cjs_runtime.h"

struct JSConstSpec {
  enum Type { Number = 0, String = 1 };

  const char* pName;
  Type eType;
  double number;
  const char* pStr;
};

struct JSPropertySpec {
  const char* pName;
  v8::AccessorNameGetterCallback pPropGet;
  v8::AccessorNameSetterCallback pPropPut;
};

struct JSMethodSpec {
  const char* pName;
  v8::FunctionCallback pMethodCall;
};

class CJS_Object : public CFXJS_PerObjectData::Binding {
 public:
  static void DefineConsts(CFXJS_Engine* pEngine,
                           uint32_t nObjDefnID,
                           pdfium::span<const JSConstSpec> consts);
  static void DefineProps(CFXJS_Engine* pEngine,
                          uint32_t nObjDefnID,
                          pdfium::span<const JSPropertySpec> consts);
  static void DefineMethods(CFXJS_Engine* pEngine,
                            uint32_t nObjDefnID,
                            pdfium::span<const JSMethodSpec> consts);

  CJS_Object(v8::Local<v8::Object> pObject, CJS_Runtime* pRuntime);

  // For simple testing only, where we need not support methods that call back
  // through an actual CJS_Runtime.
  CJS_Object(v8::Local<v8::Object> object, v8::Isolate* isolate);

  ~CJS_Object() override;

  v8::Local<v8::Object> ToV8Object() {
    return v8_object_.Get(GetRuntime()->GetIsolate());
  }
  CJS_Runtime* GetRuntime() const { return runtime_.Get(); }

 private:
  v8::Global<v8::Object> v8_object_;
  ObservedPtr<CJS_Runtime> runtime_;
};

#endif  // FXJS_CJS_OBJECT_H_
