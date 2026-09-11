// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

// CFXJS_ENGINE is a layer that makes it easier to define native objects in V8,
// but has no knowledge of PDF-specific native objects. It could in theory be
// used to implement other sets of native objects.

// PDFium code should include this file rather than including V8 headers
// directly.

#ifndef FXJS_CFXJS_ENGINE_H_
#define FXJS_CFXJS_ENGINE_H_

#include <array>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <utility>

#include "core/fxcrt/widestring.h"
#include "fxjs/cfx_isolate_wrapper.h"
#include "fxjs/ijs_runtime.h"
#include "v8/include/v8-forward.h"
#include "v8/include/v8-function-callback.h"
#include "v8/include/v8-persistent-handle.h"
#include "v8/include/v8-template.h"

class CFXJS_ObjDefinition;
class V8TemplateMap;

enum FXJSOBJTYPE {
  FXJSOBJTYPE_DYNAMIC = 0,  // Created by native method and returned to JS.
  FXJSOBJTYPE_STATIC,       // Created by init and hung off of global object.
  FXJSOBJTYPE_GLOBAL,       // The global object itself (may only appear once).
};

// Defined here to avoid circular include dependencies with js_define.h. The
// maximum must be consistent with constants in js_define.h.
inline constexpr uint32_t kNotAnFxObject = 0;
inline constexpr size_t kMaxFxObjDefinitions = 21;

class CFXJS_PerIsolateData {
 public:
  // Hook for XFA's data, when present.
  class ExtensionIface {
   public:
    virtual ~ExtensionIface() = default;
  };

  ~CFXJS_PerIsolateData();

  static CFXJS_PerIsolateData* GetOrCreate(v8::Isolate* isolate);
  static CFXJS_PerIsolateData* Get(v8::Isolate* isolate);

  CFXJS_ObjDefinition* ObjDefinitionForID(uint32_t id) const;
  void InstallObjDefinitionForID(uint32_t id,
                                 std::unique_ptr<CFXJS_ObjDefinition> defn);
  V8TemplateMap* GetDynamicObjsMap() { return dynamic_objs_map_.get(); }
  ExtensionIface* GetExtension() { return extension_.get(); }
  void SetExtension(std::unique_ptr<ExtensionIface> extension) {
    extension_ = std::move(extension);
  }
  v8::Local<v8::ObjectTemplate> GetOrCreateDefaultGlobalObjectTemplate(
      v8::Isolate* isolate);

  size_t engine_ref_count() const { return engine_ref_count_; }
  size_t IncrementEngineRefCount() {
    CHECK_LT(engine_ref_count_, std::numeric_limits<size_t>::max());
    return ++engine_ref_count_;
  }
  size_t DecrementEngineRefCount() {
    CHECK_GT(engine_ref_count_, 0u);
    return --engine_ref_count_;
  }

 private:
  explicit CFXJS_PerIsolateData(v8::Isolate* isolate);

  size_t engine_ref_count_ = 0;
  const wchar_t* const tag_;  // Raw, always a literal.
  std::array<std::unique_ptr<CFXJS_ObjDefinition>, kMaxFxObjDefinitions>
      object_defn_array_;
  std::unique_ptr<V8TemplateMap> dynamic_objs_map_;
  std::unique_ptr<ExtensionIface> extension_;
  v8::Global<v8::ObjectTemplate> default_global_object_template_;
};

class CFXJS_PerObjectData {
 public:
  // Object on the C++ side to which the v8::Object is bound.
  class Binding {
   public:
    virtual ~Binding() = default;
  };

  static void SetNewDataInObject(uint32_t obj_definition_id,
                                 v8::Local<v8::Object> obj);
  static CFXJS_PerObjectData* GetFromObject(v8::Local<v8::Object> obj);

  ~CFXJS_PerObjectData();

  uint32_t GetObjDefnID() const { return obj_definition_id_; }
  Binding* GetBinding() { return binding_.get(); }
  void SetBinding(std::unique_ptr<Binding> binding) {
    binding_ = std::move(binding);
  }

 private:
  explicit CFXJS_PerObjectData(uint32_t obj_definition_id);

  static bool HasInternalFields(v8::Local<v8::Object> obj);
  static CFXJS_PerObjectData* ExtractFromObject(v8::Local<v8::Object> obj);

  const uint32_t obj_definition_id_;
  std::unique_ptr<Binding> binding_;
};

void FXJS_Initialize(unsigned int embedder_data_slot, v8::Isolate* isolate);
void FXJS_Release();

// Gets the global isolate set by FXJS_Initialize(), or makes a new one each
// time if there is no such isolate. Returns true if a new isolate had to be
// created.
bool FXJS_GetIsolate(v8::Isolate** pResultIsolate);

// Get the global isolate's ref count.
size_t FXJS_GlobalIsolateRefCount();

class CFXJS_Engine : public CFX_IsolateWrapper {
 public:
  explicit CFXJS_Engine(v8::Isolate* isolate);
  ~CFXJS_Engine() override;

  using Constructor =
      std::function<void(CFXJS_Engine* pEngine, v8::Local<v8::Object> obj)>;
  using Destructor = std::function<void(v8::Local<v8::Object> obj)>;

  static uint32_t GetObjDefnID(v8::Local<v8::Object> obj);
  static CFXJS_PerObjectData::Binding* GetBinding(v8::Isolate* isolate,
                                                  v8::Local<v8::Object> obj);
  static void SetBinding(v8::Local<v8::Object> obj,
                         std::unique_ptr<CFXJS_PerObjectData::Binding> binding);
  static void FreePerObjectData(v8::Local<v8::Object> obj);

  void DefineObj(uint32_t obj_definition_id,
                 const char* obj_name,
                 FXJSOBJTYPE obj_type,
                 Constructor constructor,
                 Destructor destructor);

  void DefineObjMethod(uint32_t obj_definition_id,
                       const char* method_name,
                       v8::FunctionCallback method_callback);
  void DefineObjProperty(uint32_t obj_definition_id,
                         const char* prop_name,
                         v8::AccessorNameGetterCallback prop_getter,
                         v8::AccessorNameSetterCallbackV2 prop_setter);
  void DefineObjAllProperties(uint32_t obj_definition_id,
                              v8::NamedPropertyQueryCallback prop_query,
                              v8::NamedPropertyGetterCallback prop_getter,
                              v8::NamedPropertySetterCallbackV2 prop_setter,
                              v8::NamedPropertyDeleterCallback prop_deleter,
                              v8::NamedPropertyEnumeratorCallback prop_enumer);
  void DefineObjConst(uint32_t obj_definition_id,
                      const char* const_name,
                      v8::Local<v8::Value> default_value);
  void DefineGlobalMethod(const char* method_name,
                          v8::FunctionCallback method_callback);
  void DefineGlobalConst(const wchar_t* const_name,
                         v8::FunctionCallback getter_callback);

  // Called after FXJS_Define* calls made.
  void InitializeEngine();
  void ReleaseEngine();

  // Called after FXJS_InitializeEngine call made.
  std::optional<IJS_Runtime::JS_Error> Execute(const WideString& script);

  v8::Local<v8::Object> GetThisObj();
  v8::Local<v8::Object> NewFXJSBoundObject(uint32_t obj_definition_id,
                                           FXJSOBJTYPE obj_type);
  void Error(const WideString& message);

  v8::Local<v8::Context> GetV8Context();

  v8::Local<v8::Array> GetConstArray(const WideString& name);
  void SetConstArray(const WideString& name, v8::Local<v8::Array> array);

 protected:
  CFXJS_Engine();

 private:
  v8::Global<v8::Context> v8_context_;
  std::array<v8::Global<v8::Object>, kMaxFxObjDefinitions> static_objects_;
  std::map<WideString, v8::Global<v8::Array>> const_arrays_;
};

#endif  // FXJS_CFXJS_ENGINE_H_
