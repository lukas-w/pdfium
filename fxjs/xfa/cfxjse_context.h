// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_XFA_CFXJSE_CONTEXT_H_
#define FXJS_XFA_CFXJSE_CONTEXT_H_

#include <memory>
#include <vector>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/unowned_ptr.h"
#include "v8/include/cppgc/persistent.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-persistent-handle.h"

class CFXJSE_Class;
class CFXJSE_HostObject;
class CFXJSE_Value;
class CXFA_ThisProxy;
struct FXJSE_CLASS_DESCRIPTOR;

class CFXJSE_Context {
 public:
  struct ExecutionResult {
    ExecutionResult();
    ExecutionResult(bool sts, std::unique_ptr<CFXJSE_Value> val);
    ExecutionResult(ExecutionResult&& that) noexcept;
    ExecutionResult& operator=(ExecutionResult&& that) noexcept;
    ~ExecutionResult();

    bool status = false;
    std::unique_ptr<CFXJSE_Value> value;
  };

  static std::unique_ptr<CFXJSE_Context> Create(
      v8::Isolate* pIsolate,
      const FXJSE_CLASS_DESCRIPTOR* pGlobalClass,
      CFXJSE_HostObject* pGlobalObject,
      CXFA_ThisProxy* pProxy);

  ~CFXJSE_Context();

  v8::Isolate* GetIsolate() const { return isolate_; }
  v8::Local<v8::Context> GetContext();
  v8::Local<v8::Object> GetGlobalObject();

  void AddClass(std::unique_ptr<CFXJSE_Class> pClass);
  CFXJSE_Class* GetClassByName(ByteStringView szName) const;
  void EnableCompatibleMode();

  // Note: `pNewThisObject` may be empty.
  ExecutionResult ExecuteScript(ByteStringView bsScript,
                                v8::Local<v8::Object> pNewThisObject);

 private:
  CFXJSE_Context(v8::Isolate* pIsolate, CXFA_ThisProxy* pProxy);
  CFXJSE_Context(const CFXJSE_Context&) = delete;
  CFXJSE_Context& operator=(const CFXJSE_Context&) = delete;

  v8::Global<v8::Context> context_;
  UnownedPtr<v8::Isolate> isolate_;
  std::vector<std::unique_ptr<CFXJSE_Class>> classes_;
  cppgc::Persistent<CXFA_ThisProxy> this_proxy_;
};

void FXJSE_UpdateObjectBinding(v8::Local<v8::Object> hObject,
                               CFXJSE_HostObject* pNewBinding);

void FXJSE_ClearObjectBinding(v8::Local<v8::Object> hJSObject);
CFXJSE_HostObject* FXJSE_RetrieveObjectBinding(v8::Local<v8::Value> hValue);

#endif  // FXJS_XFA_CFXJSE_CONTEXT_H_
