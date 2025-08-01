// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/xfa/cfxjse_context.h"

#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/ptr_util.h"
#include "fxjs/cfxjs_engine.h"
#include "fxjs/fxv8.h"
#include "fxjs/xfa/cfxjse_class.h"
#include "fxjs/xfa/cfxjse_isolatetracker.h"
#include "fxjs/xfa/cfxjse_runtimedata.h"
#include "fxjs/xfa/cfxjse_value.h"
#include "fxjs/xfa/cjx_object.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-function.h"
#include "v8/include/v8-message.h"
#include "v8/include/v8-script.h"
#include "xfa/fxfa/parser/cxfa_thisproxy.h"

namespace {

const char szCompatibleModeScript[] =
    "(function(global, list) {\n"
    "  'use strict';\n"
    "  var objname;\n"
    "  for (objname in list) {\n"
    "    var globalobj = global[objname];\n"
    "    if (globalobj) {\n"
    "      list[objname].forEach(function(name) {\n"
    "        if (!globalobj[name]) {\n"
    "          Object.defineProperty(globalobj, name, {\n"
    "            writable: true,\n"
    "            enumerable: false,\n"
    "            value: (function(obj) {\n"
    "              if (arguments.length === 0) {\n"
    "                throw new TypeError('missing argument 0 when calling "
    "                    function ' + objname + '.' + name);\n"
    "              }\n"
    "              return globalobj.prototype[name].apply(obj, "
    "                  Array.prototype.slice.call(arguments, 1));\n"
    "            })\n"
    "          });\n"
    "        }\n"
    "      });\n"
    "    }\n"
    "  }\n"
    "}(this, {String: ['substr', 'toUpperCase']}));";

const char szConsoleScript[] =
    "console.show = function() {};\n"
    "\n"
    "console.println = function(...args) {\n"
    "  this.log(...args);\n"
    "};";

// Only address matters, values are for humans debuging here.  Keep these
// wchar_t to prevent the compiler from doing something clever, like
// aligning them on a byte boundary to save space, which would make them
// incompatible for use as V8 aligned pointers.
const wchar_t kFXJSEHostObjectTag[] = L"FXJSE Host Object";

v8::Local<v8::Object> CreateReturnValue(v8::Isolate* pIsolate,
                                        v8::TryCatch* trycatch) {
  v8::Local<v8::Object> hReturnValue = v8::Object::New(pIsolate);
  if (!trycatch->HasCaught()) {
    return hReturnValue;
  }

  v8::Local<v8::Message> hMessage = trycatch->Message();
  if (hMessage.IsEmpty()) {
    return hReturnValue;
  }

  v8::Local<v8::Context> context = pIsolate->GetCurrentContext();
  v8::Local<v8::Value> hException = trycatch->Exception();
  if (hException->IsObject()) {
    v8::Local<v8::String> hNameStr = fxv8::NewStringHelper(pIsolate, "name");
    v8::Local<v8::Value> hValue =
        hException.As<v8::Object>()->Get(context, hNameStr).ToLocalChecked();
    if (hValue->IsString() || hValue->IsStringObject()) {
      hReturnValue->Set(context, 0, hValue).FromJust();
    } else {
      v8::Local<v8::String> hErrorStr =
          fxv8::NewStringHelper(pIsolate, "Error");
      hReturnValue->Set(context, 0, hErrorStr).FromJust();
    }
    v8::Local<v8::String> hMessageStr =
        fxv8::NewStringHelper(pIsolate, "message");
    hValue =
        hException.As<v8::Object>()->Get(context, hMessageStr).ToLocalChecked();
    if (hValue->IsString() || hValue->IsStringObject()) {
      hReturnValue->Set(context, 1, hValue).FromJust();
    } else {
      hReturnValue->Set(context, 1, hMessage->Get()).FromJust();
    }
  } else {
    v8::Local<v8::String> hErrorStr = fxv8::NewStringHelper(pIsolate, "Error");
    hReturnValue->Set(context, 0, hErrorStr).FromJust();
    hReturnValue->Set(context, 1, hMessage->Get()).FromJust();
  }
  hReturnValue->Set(context, 2, hException).FromJust();
  int line = hMessage->GetLineNumber(context).FromMaybe(0);
  hReturnValue->Set(context, 3, v8::Integer::New(pIsolate, line)).FromJust();
  v8::Local<v8::String> source =
      hMessage->GetSourceLine(context).FromMaybe(v8::Local<v8::String>());
  hReturnValue->Set(context, 4, source).FromJust();
  int column = hMessage->GetStartColumn(context).FromMaybe(0);
  hReturnValue->Set(context, 5, v8::Integer::New(pIsolate, column)).FromJust();
  column = hMessage->GetEndColumn(context).FromMaybe(0);
  hReturnValue->Set(context, 6, v8::Integer::New(pIsolate, column)).FromJust();
  return hReturnValue;
}

}  // namespace

void FXJSE_UpdateObjectBinding(v8::Local<v8::Object> hObject,
                               CFXJSE_HostObject* pNewBinding) {
  DCHECK(!hObject.IsEmpty());
  DCHECK_EQ(hObject->InternalFieldCount(), 2);
  hObject->SetAlignedPointerInInternalField(
      0, const_cast<wchar_t*>(kFXJSEHostObjectTag));
  hObject->SetAlignedPointerInInternalField(1, pNewBinding);
}

void FXJSE_ClearObjectBinding(v8::Local<v8::Object> hObject) {
  DCHECK(!hObject.IsEmpty());
  DCHECK_EQ(hObject->InternalFieldCount(), 2);
  hObject->SetAlignedPointerInInternalField(0, nullptr);
  hObject->SetAlignedPointerInInternalField(1, nullptr);
}

CFXJSE_HostObject* FXJSE_RetrieveObjectBinding(v8::Local<v8::Value> hValue) {
  if (!fxv8::IsObject(hValue)) {
    return nullptr;
  }

  v8::Local<v8::Object> hObject = hValue.As<v8::Object>();
  if (hObject->InternalFieldCount() != 2 ||
      hObject->GetAlignedPointerFromInternalField(0) != kFXJSEHostObjectTag) {
    return nullptr;
  }

  return static_cast<CFXJSE_HostObject*>(
      hObject->GetAlignedPointerFromInternalField(1));
}

// static
std::unique_ptr<CFXJSE_Context> CFXJSE_Context::Create(
    v8::Isolate* pIsolate,
    const FXJSE_CLASS_DESCRIPTOR* pGlobalClass,
    CFXJSE_HostObject* pGlobalObject,
    CXFA_ThisProxy* pProxy) {
  CFXJSE_ScopeUtil_IsolateHandle scope(pIsolate);

  // Private constructor.
  auto pContext = pdfium::WrapUnique(new CFXJSE_Context(pIsolate, pProxy));
  v8::Local<v8::ObjectTemplate> hObjectTemplate;
  if (pGlobalClass) {
    CFXJSE_Class* pGlobalClassObj =
        CFXJSE_Class::Create(pContext.get(), pGlobalClass, true);
    hObjectTemplate =
        pGlobalClassObj->GetTemplate(pIsolate)->InstanceTemplate();
  } else {
    hObjectTemplate = v8::ObjectTemplate::New(pIsolate);
    hObjectTemplate->SetInternalFieldCount(2);
  }
  hObjectTemplate->Set(v8::Symbol::GetToStringTag(pIsolate),
                       fxv8::NewStringHelper(pIsolate, "global"));

  v8::Local<v8::Context> hNewContext =
      v8::Context::New(pIsolate, nullptr, hObjectTemplate);
  v8::Local<v8::Object> pThis = hNewContext->Global();
  FXJSE_UpdateObjectBinding(pThis, pGlobalObject);

  v8::Local<v8::Context> hRootContext =
      CFXJSE_RuntimeData::Get(pIsolate)->GetRootContext(pIsolate);
  hNewContext->SetSecurityToken(hRootContext->GetSecurityToken());
  pContext->context_.Reset(pIsolate, hNewContext);
  return pContext;
}

CFXJSE_Context::CFXJSE_Context(v8::Isolate* pIsolate, CXFA_ThisProxy* pProxy)
    : isolate_(pIsolate), this_proxy_(pProxy) {}

CFXJSE_Context::~CFXJSE_Context() = default;

v8::Local<v8::Object> CFXJSE_Context::GetGlobalObject() {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::EscapableHandleScope handle_scope(GetIsolate());
  v8::Local<v8::Context> hContext =
      v8::Local<v8::Context>::New(GetIsolate(), context_);
  v8::Local<v8::Object> result = hContext->Global();
  return handle_scope.Escape(result);
}

v8::Local<v8::Context> CFXJSE_Context::GetContext() {
  return v8::Local<v8::Context>::New(GetIsolate(), context_);
}

void CFXJSE_Context::AddClass(std::unique_ptr<CFXJSE_Class> pClass) {
  classes_.push_back(std::move(pClass));
}

CFXJSE_Class* CFXJSE_Context::GetClassByName(ByteStringView szName) const {
  auto pClass = std::ranges::find_if(
      classes_, [szName](const std::unique_ptr<CFXJSE_Class>& item) {
        return item->IsName(szName);
      });
  return pClass != classes_.end() ? pClass->get() : nullptr;
}

void CFXJSE_Context::EnableCompatibleMode() {
  ExecuteScript(szCompatibleModeScript, v8::Local<v8::Object>());
  ExecuteScript(szConsoleScript, v8::Local<v8::Object>());
}

CFXJSE_Context::ExecutionResult CFXJSE_Context::ExecuteScript(
    ByteStringView bsScript,
    v8::Local<v8::Object> hNewThis) {
  CFXJSE_ScopeUtil_IsolateHandleContext scope(this);
  v8::Local<v8::Context> hContext = GetIsolate()->GetCurrentContext();
  v8::TryCatch trycatch(GetIsolate());
  v8::Local<v8::String> hScriptString =
      fxv8::NewStringHelper(GetIsolate(), bsScript);
  if (hNewThis.IsEmpty()) {
    v8::Local<v8::Script> hScript;
    if (v8::Script::Compile(hContext, hScriptString).ToLocal(&hScript)) {
      CHECK(!trycatch.HasCaught());
      v8::Local<v8::Value> hValue;
      if (hScript->Run(hContext).ToLocal(&hValue)) {
        CHECK(!trycatch.HasCaught());
        return ExecutionResult(
            true, std::make_unique<CFXJSE_Value>(GetIsolate(), hValue));
      }
    }
    return ExecutionResult(
        false, std::make_unique<CFXJSE_Value>(
                   GetIsolate(), CreateReturnValue(GetIsolate(), &trycatch)));
  }

  v8::Local<v8::String> hEval = fxv8::NewStringHelper(
      GetIsolate(), "(function () { return eval(arguments[0]); })");
  v8::Local<v8::Script> hWrapper =
      v8::Script::Compile(hContext, hEval).ToLocalChecked();
  v8::Local<v8::Value> hWrapperValue;
  if (hWrapper->Run(hContext).ToLocal(&hWrapperValue)) {
    CHECK(!trycatch.HasCaught());
    CHECK(hWrapperValue->IsFunction());
    v8::Local<v8::Function> hWrapperFn = hWrapperValue.As<v8::Function>();
    v8::Local<v8::Value> rgArgs[] = {hScriptString};
    v8::Local<v8::Value> hValue;
    if (hWrapperFn->Call(hContext, hNewThis, 1, rgArgs).ToLocal(&hValue)) {
      DCHECK(!trycatch.HasCaught());
      return ExecutionResult(
          true, std::make_unique<CFXJSE_Value>(GetIsolate(), hValue));
    }
  }

#ifndef NDEBUG
  v8::String::Utf8Value error(GetIsolate(), trycatch.Exception());
  UNSAFE_TODO(fprintf(stderr, "JS Error: %s\n", *error));

  v8::Local<v8::Message> message = trycatch.Message();
  if (!message.IsEmpty()) {
    v8::Local<v8::Context> context(GetIsolate()->GetCurrentContext());
    int linenum = message->GetLineNumber(context).FromJust();
    v8::String::Utf8Value sourceline(
        GetIsolate(), message->GetSourceLine(context).ToLocalChecked());
    UNSAFE_TODO(fprintf(stderr, "Line %d: %s\n", linenum, *sourceline));
  }
#endif  // NDEBUG

  return ExecutionResult(
      false, std::make_unique<CFXJSE_Value>(
                 GetIsolate(), CreateReturnValue(GetIsolate(), &trycatch)));
}

CFXJSE_Context::ExecutionResult::ExecutionResult() = default;

CFXJSE_Context::ExecutionResult::ExecutionResult(
    bool sts,
    std::unique_ptr<CFXJSE_Value> val)
    : status(sts), value(std::move(val)) {}

CFXJSE_Context::ExecutionResult::ExecutionResult(
    ExecutionResult&& that) noexcept = default;

CFXJSE_Context::ExecutionResult& CFXJSE_Context::ExecutionResult::operator=(
    ExecutionResult&& that) noexcept = default;

CFXJSE_Context::ExecutionResult::~ExecutionResult() = default;
