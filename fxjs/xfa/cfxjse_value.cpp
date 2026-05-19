// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cfxjse_value.h"

#include <math.h>

#include "core/fxcrt/check.h"
#include "fxjs/fxv8.h"
#include "fxjs/xfa/cfxjse_class.h"
#include "fxjs/xfa/cfxjse_context.h"
#include "fxjs/xfa/cfxjse_isolatetracker.h"
#include "v8/include/v8-container.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-local-handle.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-script.h"

void FXJSE_ThrowMessage(v8::Isolate* pIsolate, ByteStringView utf8Message) {
  DCHECK(pIsolate);
  CFXJSE_ScopeUtil_IsolateHandleRootContext scope(pIsolate);
  v8::Local<v8::String> hMessage = fxv8::NewStringHelper(pIsolate, utf8Message);
  v8::Local<v8::Value> hError = v8::Exception::Error(hMessage);
  pIsolate->ThrowException(hError);
}

CFXJSE_Value::CFXJSE_Value() = default;

CFXJSE_Value::CFXJSE_Value(v8::Isolate* pIsolate, v8::Local<v8::Value> value) {
  ForceSetValue(pIsolate, value);
}

CFXJSE_Value::~CFXJSE_Value() = default;

CFXJSE_HostObject* CFXJSE_Value::ToHostObject(v8::Isolate* pIsolate) const {
  CFXJSE_ScopeUtil_IsolateHandleRootContext scope(pIsolate);
  return CFXJSE_HostObject::FromV8(v8::Local<v8::Value>::New(pIsolate, value_));
}

void CFXJSE_Value::SetArray(
    v8::Isolate* pIsolate,
    const std::vector<std::unique_ptr<CFXJSE_Value>>& values) {
  CFXJSE_ScopeUtil_IsolateHandleRootContext scope(pIsolate);
  v8::LocalVector<v8::Value> local_values(pIsolate);
  local_values.reserve(values.size());
  for (auto& v : values) {
    if (v->IsEmpty()) {
      local_values.push_back(fxv8::NewUndefinedHelper(pIsolate));
    } else {
      local_values.push_back(v->GetValue(pIsolate));
    }
  }
  v8::Local<v8::Array> hArrayObject =
      v8::Array::New(pIsolate, local_values.data(), local_values.size());
  value_.Reset(pIsolate, hArrayObject);
}

v8::Local<v8::Function> CFXJSE_Value::NewBoundFunction(
    v8::Isolate* pIsolate,
    v8::Local<v8::Function> hOldFunction,
    v8::Local<v8::Object> hNewThis) {
  DCHECK(!hOldFunction.IsEmpty());
  DCHECK(!hNewThis.IsEmpty());

  CFXJSE_ScopeUtil_RootContext scope(pIsolate);
  v8::Local<v8::Value> rgArgs[2];
  rgArgs[0] = hOldFunction;
  rgArgs[1] = hNewThis;
  v8::Local<v8::String> hBinderFuncSource = fxv8::NewStringHelper(
      pIsolate, "(function (fn, obj) { return fn.bind(obj); })");
  v8::Local<v8::Context> hContext = pIsolate->GetCurrentContext();
  v8::Local<v8::Function> hBinderFunc =
      v8::Script::Compile(hContext, hBinderFuncSource)
          .ToLocalChecked()
          ->Run(hContext)
          .ToLocalChecked()
          .As<v8::Function>();
  v8::Local<v8::Value> hBoundFunction =
      hBinderFunc->Call(hContext, hContext->Global(), 2, rgArgs)
          .ToLocalChecked();
  if (!fxv8::IsFunction(hBoundFunction)) {
    return v8::Local<v8::Function>();
  }

  return hBoundFunction.As<v8::Function>();
}

v8::Local<v8::Value> CFXJSE_Value::GetValue(v8::Isolate* pIsolate) const {
  return v8::Local<v8::Value>::New(pIsolate, value_);
}

bool CFXJSE_Value::IsEmpty() const {
  return value_.IsEmpty();
}

bool CFXJSE_Value::IsUndefined(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsUndefined();
}

bool CFXJSE_Value::IsNull(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsNull();
}

bool CFXJSE_Value::IsBoolean(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsBoolean();
}

bool CFXJSE_Value::IsString(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsString();
}

bool CFXJSE_Value::IsNumber(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsNumber();
}

bool CFXJSE_Value::IsObject(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsObject();
}

bool CFXJSE_Value::IsArray(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsArray();
}

bool CFXJSE_Value::IsFunction(v8::Isolate* pIsolate) const {
  if (IsEmpty()) {
    return false;
  }

  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  v8::Local<v8::Value> hValue = v8::Local<v8::Value>::New(pIsolate, value_);
  return hValue->IsFunction();
}

bool CFXJSE_Value::ToBoolean(v8::Isolate* pIsolate) const {
  DCHECK(!IsEmpty());
  CFXJSE_ScopeUtil_IsolateHandleRootContext scope(pIsolate);
  return fxv8::ReentrantToBooleanHelper(
      pIsolate, v8::Local<v8::Value>::New(pIsolate, value_));
}

ByteString CFXJSE_Value::ToString(v8::Isolate* pIsolate) const {
  DCHECK(!IsEmpty());
  CFXJSE_ScopeUtil_IsolateHandleRootContext scope(pIsolate);
  return fxv8::ReentrantToByteStringHelper(
      pIsolate, v8::Local<v8::Value>::New(pIsolate, value_));
}

void CFXJSE_Value::SetUndefined(v8::Isolate* pIsolate) {
  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);
  value_.Reset(pIsolate, fxv8::NewUndefinedHelper(pIsolate));
}
