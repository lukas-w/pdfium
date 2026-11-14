// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/fxjse.h"

#include "fxjs/fxv8.h"
#include "fxjs/xfa/cfxjse_context.h"
#include "fxjs/xfa/cfxjse_isolatetracker.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-object.h"

namespace pdfium::fxjse {

const char kFuncTag[] = "function descriptor tag";
const char kClassTag[] = "class descriptor tag";

}  // namespace pdfium::fxjse

// static
CFXJSE_HostObject* CFXJSE_HostObject::FromV8(v8::Local<v8::Value> arg) {
  if (!fxv8::IsObject(arg)) {
    return nullptr;
  }

  return FXJSE_RetrieveObjectBinding(arg.As<v8::Object>());
}

CFXJSE_HostObject::CFXJSE_HostObject() = default;

CFXJSE_HostObject::~CFXJSE_HostObject() = default;

CFXJSE_FormCalcContext* CFXJSE_HostObject::AsFormCalcContext() {
  return nullptr;
}

CJX_Object* CFXJSE_HostObject::AsCJXObject() {
  return nullptr;
}

void FXJSE_ThrowMessage(v8::Isolate* pIsolate, ByteStringView utf8Message) {
  DCHECK(pIsolate);
  CFXJSE_ScopeUtil_IsolateHandleRootContext scope(pIsolate);
  v8::Local<v8::String> hMessage = fxv8::NewStringHelper(pIsolate, utf8Message);
  v8::Local<v8::Value> hError = v8::Exception::Error(hMessage);
  pIsolate->ThrowException(hError);
}
