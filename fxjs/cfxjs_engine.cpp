// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cfxjs_engine.h"

#include <memory>
#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "fxjs/cfx_v8_array_buffer_allocator.h"
#include "fxjs/cjs_object.h"
#include "fxjs/fxv8.h"
#include "fxjs/xfa/cfxjse_runtimedata.h"
#include "v8/include/v8-context.h"
#include "v8/include/v8-exception.h"
#include "v8/include/v8-isolate.h"
#include "v8/include/v8-message.h"
#include "v8/include/v8-primitive.h"
#include "v8/include/v8-script.h"
#include "v8/include/v8-util.h"

namespace {

unsigned int g_embedderDataSlot = 1u;
v8::Isolate* g_isolate = nullptr;

// Only the address matters, values are for humans debugging. ASLR should
// ensure that these values are unlikely to arise otherwise. Keep these
// wchar_t to prevent the compiler from doing something clever, like
// aligning them on a byte boundary to save space, which would make them
// incompatible for use as V8 aligned pointers.
const wchar_t kPerObjectDataTag[] = L"CFXJS_PerObjectData";
const wchar_t kPerIsolateDataTag[] = L"CFXJS_PerIsolateData";

void* GetAlignedPointerForPerObjectDataTag() {
  return const_cast<void*>(static_cast<const void*>(kPerObjectDataTag));
}

std::pair<int, int> GetLineAndColumnFromError(v8::Local<v8::Message> message,
                                              v8::Local<v8::Context> context) {
  if (message.IsEmpty()) {
    return std::make_pair(-1, -1);
  }
  return std::make_pair(message->GetLineNumber(context).FromMaybe(-1),
                        message->GetStartColumn());
}

}  // namespace

// static
void CFXJS_PerObjectData::SetNewDataInObject(uint32_t obj_definition_id,
                                             v8::Local<v8::Object> obj) {
  if (obj->InternalFieldCount() == 2) {
    obj->SetAlignedPointerInInternalField(
        0, GetAlignedPointerForPerObjectDataTag(), fxv8::kPDFiumSentinelTag);
    obj->SetAlignedPointerInInternalField(
        1, new CFXJS_PerObjectData(obj_definition_id),
        fxv8::kFXJSPerObjectDataTag);
  }
}

// static
CFXJS_PerObjectData* CFXJS_PerObjectData::GetFromObject(
    v8::Local<v8::Object> obj) {
  if (obj.IsEmpty()) {
    return nullptr;
  }
  if (!HasInternalFields(obj)) {
    return nullptr;
  }
  return ExtractFromObject(obj);
}

//  static
bool CFXJS_PerObjectData::HasInternalFields(v8::Local<v8::Object> obj) {
  return obj->InternalFieldCount() == 2 &&
         obj->GetAlignedPointerFromInternalField(0, fxv8::kPDFiumSentinelTag) ==
             GetAlignedPointerForPerObjectDataTag();
}

//  static
CFXJS_PerObjectData* CFXJS_PerObjectData::ExtractFromObject(
    v8::Local<v8::Object> obj) {
  return static_cast<CFXJS_PerObjectData*>(
      obj->GetAlignedPointerFromInternalField(1, fxv8::kFXJSPerObjectDataTag));
}

CFXJS_PerObjectData::CFXJS_PerObjectData(uint32_t obj_definition_id)
    : obj_definition_id_(obj_definition_id) {}

CFXJS_PerObjectData::~CFXJS_PerObjectData() = default;

// Global weak map to save dynamic objects.
class V8TemplateMapTraits final
    : public v8::StdMapTraits<CFXJS_PerObjectData*, v8::Object> {
 public:
  using WeakCallbackDataType = CFXJS_PerObjectData;
  using MapType = v8::
      GlobalValueMap<WeakCallbackDataType*, v8::Object, V8TemplateMapTraits>;

  static const v8::PersistentContainerCallbackType kCallbackType =
      v8::kWeakWithInternalFields;

  static WeakCallbackDataType* WeakCallbackParameter(
      MapType* map,
      WeakCallbackDataType* key,
      v8::Local<v8::Object> value) {
    return key;
  }
  static MapType* MapFromWeakCallbackInfo(
      const v8::WeakCallbackInfo<WeakCallbackDataType>&);
  static WeakCallbackDataType* KeyFromWeakCallbackInfo(
      const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
    return data.GetParameter();
  }
  static void OnWeakCallback(
      const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {}
  static void DisposeWeak(
      const v8::WeakCallbackInfo<WeakCallbackDataType>& data);
  static void Dispose(v8::Isolate* isolate,
                      v8::Global<v8::Object> value,
                      WeakCallbackDataType* key);
  static void DisposeCallbackData(WeakCallbackDataType* callbackData) {}
};

class V8TemplateMap {
 public:
  using WeakCallbackDataType = CFXJS_PerObjectData;
  using MapType = v8::
      GlobalValueMap<WeakCallbackDataType*, v8::Object, V8TemplateMapTraits>;

  explicit V8TemplateMap(v8::Isolate* isolate) : map_(isolate) {}
  ~V8TemplateMap() = default;

  void SetAndMakeWeak(v8::Local<v8::Object> handle) {
    WeakCallbackDataType* key = CFXJS_PerObjectData::GetFromObject(handle);
    DCHECK(!map_.Contains(key));

    // Inserting an object into a GlobalValueMap with the appropriate traits
    // has the side-effect of making the object weak deep in the guts of V8,
    // and arranges for it to be cleaned up by the methods in the traits.
    map_.Set(key, handle);
  }

  MapType* GetMap() { return &map_; }

 private:
  MapType map_;
};

class CFXJS_ObjDefinition {
 public:
  CFXJS_ObjDefinition(v8::Isolate* isolate,
                      const char* obj_name,
                      FXJSOBJTYPE obj_type,
                      CFXJS_Engine::Constructor constructor,
                      CFXJS_Engine::Destructor destructor)
      : obj_name_(obj_name),
        obj_type_(obj_type),
        constructor_(constructor),
        destructor_(destructor),
        isolate_(isolate) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::FunctionTemplate> fn = v8::FunctionTemplate::New(isolate);
    fn->InstanceTemplate()->SetInternalFieldCount(2);
    fn->InstanceTemplate()->SetImmutableProto();
    fn->SetCallHandler(CallHandler, v8::Number::New(isolate, obj_type));
    if (obj_type == FXJSOBJTYPE_GLOBAL) {
      fn->InstanceTemplate()->Set(v8::Symbol::GetToStringTag(isolate),
                                  fxv8::NewStringHelper(isolate, "global"));
    }
    function_template_.Reset(isolate, fn);
    signature_.Reset(isolate, v8::Signature::New(isolate, fn));
  }

  static void CallHandler(const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    if (!info.IsConstructCall()) {
      fxv8::ThrowExceptionHelper(isolate, "illegal constructor");
      return;
    }
    if (info.Data().As<v8::Int32>()->Value() != FXJSOBJTYPE_DYNAMIC) {
      fxv8::ThrowExceptionHelper(isolate, "not a dynamic object");
      return;
    }
    v8::Local<v8::Object> holder = info.This();
    DCHECK_EQ(holder->InternalFieldCount(), 2);
    holder->SetAlignedPointerInInternalField(0, nullptr,
                                             fxv8::kPDFiumSentinelTag);
    holder->SetAlignedPointerInInternalField(1, nullptr,
                                             fxv8::kFXJSPerObjectDataTag);
  }

  FXJSOBJTYPE GetObjType() const { return obj_type_; }
  const char* GetObjName() const { return obj_name_; }
  v8::Isolate* GetIsolate() const { return isolate_; }

  void DefineConst(const char* const_name, v8::Local<v8::Value> default_value) {
    GetInstanceTemplate()->Set(GetIsolate(), const_name, default_value);
  }

  void DefineProperty(v8::Local<v8::String> prop_name,
                      v8::AccessorNameGetterCallback prop_getter,
                      v8::AccessorNameSetterCallbackV2 prop_setter) {
    GetInstanceTemplate()->SetNativeDataProperty(prop_name, prop_getter,
                                                 prop_setter);
  }

  void DefineMethod(v8::Local<v8::String> method_name,
                    v8::FunctionCallback method_callback) {
    v8::Local<v8::FunctionTemplate> fun = v8::FunctionTemplate::New(
        GetIsolate(), method_callback, v8::Local<v8::Value>(), GetSignature());
    fun->RemovePrototype();
    GetInstanceTemplate()->Set(method_name, fun, v8::ReadOnly);
  }

  void DefineAllProperties(v8::NamedPropertyQueryCallback prop_query,
                           v8::NamedPropertyGetterCallback prop_getter,
                           v8::NamedPropertySetterCallbackV2 prop_setter,
                           v8::NamedPropertyDeleterCallback prop_deleter,
                           v8::NamedPropertyEnumeratorCallback prop_enumer) {
    GetInstanceTemplate()->SetHandler(v8::NamedPropertyHandlerConfiguration(
        prop_getter, prop_setter, prop_query, prop_deleter, prop_enumer,
        v8::Local<v8::Value>(),
        v8::PropertyHandlerFlags::kOnlyInterceptStrings));
  }

  v8::Local<v8::ObjectTemplate> GetInstanceTemplate() {
    v8::EscapableHandleScope scope(GetIsolate());
    v8::Local<v8::FunctionTemplate> function =
        function_template_.Get(GetIsolate());
    return scope.Escape(function->InstanceTemplate());
  }

  v8::Local<v8::Signature> GetSignature() {
    v8::EscapableHandleScope scope(GetIsolate());
    return scope.Escape(signature_.Get(GetIsolate()));
  }

  void RunConstructor(CFXJS_Engine* engine, v8::Local<v8::Object> obj) {
    if (constructor_) {
      constructor_(engine, obj);
    }
  }

  void RunDestructor(v8::Local<v8::Object> obj) {
    if (destructor_) {
      destructor_(obj);
    }
  }

 private:
  UnownedPtr<const char> const obj_name_;
  const FXJSOBJTYPE obj_type_;
  const CFXJS_Engine::Constructor constructor_;
  const CFXJS_Engine::Destructor destructor_;
  UnownedPtr<v8::Isolate> isolate_;
  v8::Global<v8::FunctionTemplate> function_template_;
  v8::Global<v8::Signature> signature_;
};

static v8::Local<v8::ObjectTemplate> GetGlobalObjectTemplate(
    v8::Isolate* isolate) {
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(isolate);
  for (uint32_t i = 1; i < kMaxFxObjDefinitions; ++i) {
    CFXJS_ObjDefinition* obj_defn = isolate_data->ObjDefinitionForID(i);
    if (obj_defn && obj_defn->GetObjType() == FXJSOBJTYPE_GLOBAL) {
      return obj_defn->GetInstanceTemplate();
    }
  }
  return isolate_data->GetOrCreateDefaultGlobalObjectTemplate(isolate);
}

void V8TemplateMapTraits::Dispose(v8::Isolate* isolate,
                                  v8::Global<v8::Object> value,
                                  WeakCallbackDataType* key) {
  v8::Local<v8::Object> obj = value.Get(isolate);
  if (obj.IsEmpty()) {
    return;
  }
  uint32_t id = CFXJS_Engine::GetObjDefnID(obj);
  if (id == kNotAnFxObject) {
    return;
  }
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(isolate);
  CFXJS_ObjDefinition* obj_defn = isolate_data->ObjDefinitionForID(id);
  if (!obj_defn) {
    return;
  }
  obj_defn->RunDestructor(obj);
  CFXJS_Engine::FreePerObjectData(obj);
}

void V8TemplateMapTraits::DisposeWeak(
    const v8::WeakCallbackInfo<WeakCallbackDataType>& data) {
  // TODO(tsepez): this is expected be called during GC.
}

V8TemplateMapTraits::MapType* V8TemplateMapTraits::MapFromWeakCallbackInfo(
    const v8::WeakCallbackInfo<WeakCallbackDataType>& info) {
  auto* isolate_data = CFXJS_PerIsolateData::Get(info.GetIsolate());
  V8TemplateMap* objs_map = isolate_data->GetDynamicObjsMap();
  return objs_map ? objs_map->GetMap() : nullptr;
}

void FXJS_Initialize(unsigned int embedder_data_slot, v8::Isolate* isolate) {
  if (g_isolate) {
    DCHECK_EQ(g_embedderDataSlot, embedder_data_slot);
    DCHECK_EQ(g_isolate, isolate);
    return;
  }
  g_embedderDataSlot = embedder_data_slot;
  g_isolate = isolate;
}

void FXJS_Release() {
  DCHECK(!g_isolate || !CFXJS_PerIsolateData::Get(g_isolate) ||
         CFXJS_PerIsolateData::Get(g_isolate)->engine_ref_count() == 0);
  g_isolate = nullptr;
}

bool FXJS_GetIsolate(v8::Isolate** pResultIsolate) {
  if (g_isolate) {
    *pResultIsolate = g_isolate;
    return false;
  }
  // Provide backwards compatibility when no external isolate.
  v8::Isolate::CreateParams params;
  params.array_buffer_allocator =
      CFX_V8ArrayBufferAllocator::GetSharedInstance();
  *pResultIsolate = v8::Isolate::New(params);
  return true;
}

size_t FXJS_GlobalIsolateRefCount() {
  if (!g_isolate) {
    return 0;
  }
  auto* isolate_data = CFXJS_PerIsolateData::Get(g_isolate);
  return isolate_data ? isolate_data->engine_ref_count() : 0;
}

// static
CFXJS_PerIsolateData* CFXJS_PerIsolateData::GetOrCreate(v8::Isolate* isolate) {
  auto* result = Get(isolate);
  if (result) {
    return result;
  }
  result = new CFXJS_PerIsolateData(isolate);
  isolate->SetData(g_embedderDataSlot, result);
  return result;
}

// static
CFXJS_PerIsolateData* CFXJS_PerIsolateData::Get(v8::Isolate* isolate) {
  auto* result =
      static_cast<CFXJS_PerIsolateData*>(isolate->GetData(g_embedderDataSlot));
  if (!result) {
    return nullptr;
  }
  CHECK(result->tag_ == kPerIsolateDataTag);
  return result;
}

CFXJS_PerIsolateData::CFXJS_PerIsolateData(v8::Isolate* isolate)
    : tag_(kPerIsolateDataTag),
      dynamic_objs_map_(std::make_unique<V8TemplateMap>(isolate)) {}

CFXJS_PerIsolateData::~CFXJS_PerIsolateData() = default;

CFXJS_ObjDefinition* CFXJS_PerIsolateData::ObjDefinitionForID(
    uint32_t id) const {
  return id < object_defn_array_.size() ? object_defn_array_[id].get()
                                        : nullptr;
}

void CFXJS_PerIsolateData::InstallObjDefinitionForID(
    uint32_t id,
    std::unique_ptr<CFXJS_ObjDefinition> defn) {
  CHECK_NE(id, kNotAnFxObject);
  CHECK(!object_defn_array_[id]);
  object_defn_array_[id] = std::move(defn);
}

v8::Local<v8::ObjectTemplate>
CFXJS_PerIsolateData::GetOrCreateDefaultGlobalObjectTemplate(
    v8::Isolate* isolate) {
  if (default_global_object_template_.IsEmpty()) {
    v8::Local<v8::ObjectTemplate> hGlobalTemplate =
        v8::ObjectTemplate::New(isolate);
    hGlobalTemplate->Set(v8::Symbol::GetToStringTag(isolate),
                         fxv8::NewStringHelper(isolate, "global"));
    default_global_object_template_.Reset(isolate, hGlobalTemplate);
  }
  return default_global_object_template_.Get(isolate);
}

CFXJS_Engine::CFXJS_Engine() : CFX_IsolateWrapper(nullptr) {}

CFXJS_Engine::CFXJS_Engine(v8::Isolate* isolate)
    : CFX_IsolateWrapper(isolate) {}

CFXJS_Engine::~CFXJS_Engine() = default;

// static
uint32_t CFXJS_Engine::GetObjDefnID(v8::Local<v8::Object> obj) {
  CFXJS_PerObjectData* data = CFXJS_PerObjectData::GetFromObject(obj);
  return data ? data->GetObjDefnID() : kNotAnFxObject;
}

// static
void CFXJS_Engine::SetBinding(
    v8::Local<v8::Object> obj,
    std::unique_ptr<CFXJS_PerObjectData::Binding> binding) {
  CFXJS_PerObjectData* data = CFXJS_PerObjectData::GetFromObject(obj);
  if (data) {
    data->SetBinding(std::move(binding));
  }
}

// static
void CFXJS_Engine::FreePerObjectData(v8::Local<v8::Object> obj) {
  CFXJS_PerObjectData* data = CFXJS_PerObjectData::GetFromObject(obj);
  obj->SetAlignedPointerInInternalField(0, nullptr, fxv8::kPDFiumSentinelTag);
  obj->SetAlignedPointerInInternalField(1, nullptr,
                                        fxv8::kFXJSPerObjectDataTag);
  delete data;
}

void CFXJS_Engine::DefineObj(uint32_t obj_definition_id,
                             const char* obj_name,
                             FXJSOBJTYPE obj_type,
                             CFXJS_Engine::Constructor constructor,
                             CFXJS_Engine::Destructor destructor) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_PerIsolateData* isolate_data =
      CFXJS_PerIsolateData::GetOrCreate(GetIsolate());
  isolate_data->InstallObjDefinitionForID(
      obj_definition_id,
      std::make_unique<CFXJS_ObjDefinition>(GetIsolate(), obj_name, obj_type,
                                            constructor, destructor));
}

void CFXJS_Engine::DefineObjMethod(uint32_t obj_definition_id,
                                   const char* method_name,
                                   v8::FunctionCallback method_callback) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(GetIsolate());
  CFXJS_ObjDefinition* obj_defn =
      isolate_data->ObjDefinitionForID(obj_definition_id);
  obj_defn->DefineMethod(NewString(method_name), method_callback);
}

void CFXJS_Engine::DefineObjProperty(
    uint32_t obj_definition_id,
    const char* prop_name,
    v8::AccessorNameGetterCallback prop_getter,
    v8::AccessorNameSetterCallbackV2 prop_setter) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(GetIsolate());
  CFXJS_ObjDefinition* obj_defn =
      isolate_data->ObjDefinitionForID(obj_definition_id);
  obj_defn->DefineProperty(NewString(prop_name), prop_getter, prop_setter);
}

void CFXJS_Engine::DefineObjAllProperties(
    uint32_t obj_definition_id,
    v8::NamedPropertyQueryCallback prop_query,
    v8::NamedPropertyGetterCallback prop_getter,
    v8::NamedPropertySetterCallbackV2 prop_setter,
    v8::NamedPropertyDeleterCallback prop_deleter,
    v8::NamedPropertyEnumeratorCallback prop_enumer) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(GetIsolate());
  CFXJS_ObjDefinition* obj_defn =
      isolate_data->ObjDefinitionForID(obj_definition_id);
  obj_defn->DefineAllProperties(prop_query, prop_getter, prop_setter,
                                prop_deleter, prop_enumer);
}

void CFXJS_Engine::DefineObjConst(uint32_t obj_definition_id,
                                  const char* const_name,
                                  v8::Local<v8::Value> default_value) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(GetIsolate());
  CFXJS_ObjDefinition* obj_defn =
      isolate_data->ObjDefinitionForID(obj_definition_id);
  obj_defn->DefineConst(const_name, default_value);
}

void CFXJS_Engine::DefineGlobalMethod(const char* method_name,
                                      v8::FunctionCallback method_callback) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::FunctionTemplate> fun =
      v8::FunctionTemplate::New(GetIsolate(), method_callback);
  fun->RemovePrototype();
  GetGlobalObjectTemplate(GetIsolate())
      ->Set(NewString(method_name), fun, v8::ReadOnly);
}

void CFXJS_Engine::DefineGlobalConst(const wchar_t* const_name,
                                     v8::FunctionCallback getter_callback) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::FunctionTemplate> fun =
      v8::FunctionTemplate::New(GetIsolate(), getter_callback);
  fun->RemovePrototype();
  GetGlobalObjectTemplate(GetIsolate())
      ->SetAccessorProperty(NewString(const_name), fun);
}

void CFXJS_Engine::InitializeEngine() {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());

  // This has to happen before we call GetGlobalObjectTemplate because that
  // method gets the PerIsolateData from GetIsolate().
  CFXJS_PerIsolateData* isolate_data =
      CFXJS_PerIsolateData::GetOrCreate(GetIsolate());
  isolate_data->IncrementEngineRefCount();

  v8::Local<v8::Context> v8Context = v8::Context::New(
      GetIsolate(), nullptr, GetGlobalObjectTemplate(GetIsolate()));

  // May not have the internal fields when called from tests, so clear these
  // in case we don't process a FXJSOBJTYPE_GLOBAL below.
  v8::Local<v8::Object> pThis = v8Context->Global();
  if (pThis->InternalFieldCount() == 2) {
    pThis->SetAlignedPointerInInternalField(0, nullptr,
                                            fxv8::kPDFiumSentinelTag);
    pThis->SetAlignedPointerInInternalField(1, nullptr,
                                            fxv8::kFXJSPerObjectDataTag);
  }

  v8::Context::Scope context_scope(v8Context);
  for (uint32_t i = 1; i < kMaxFxObjDefinitions; ++i) {
    CFXJS_ObjDefinition* obj_defn = isolate_data->ObjDefinitionForID(i);
    if (!obj_defn) {
      continue;
    }
    if (obj_defn->GetObjType() == FXJSOBJTYPE_GLOBAL) {
      CFXJS_PerObjectData::SetNewDataInObject(i, pThis);
      obj_defn->RunConstructor(this, pThis);

    } else if (obj_defn->GetObjType() == FXJSOBJTYPE_STATIC) {
      v8::Local<v8::String> obj_name = NewString(obj_defn->GetObjName());
      v8::Local<v8::Object> obj = NewFXJSBoundObject(i, FXJSOBJTYPE_STATIC);
      if (!obj.IsEmpty()) {
        v8Context->Global()->Set(v8Context, obj_name, obj).FromJust();
        static_objects_[i] = v8::Global<v8::Object>(GetIsolate(), obj);
      }
    }
  }

  v8_context_.Reset(GetIsolate(), v8Context);
}

void CFXJS_Engine::ReleaseEngine() {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::HandleScope handle_scope(GetIsolate());
  v8::Local<v8::Context> context = GetV8Context();
  v8::Context::Scope context_scope(context);
  CFXJS_PerIsolateData* isolate_data = CFXJS_PerIsolateData::Get(GetIsolate());
  if (!isolate_data) {
    return;
  }

  const_arrays_.clear();

  for (uint32_t i = 1; i < kMaxFxObjDefinitions; ++i) {
    CFXJS_ObjDefinition* obj_defn = isolate_data->ObjDefinitionForID(i);
    if (!obj_defn) {
      continue;
    }
    v8::Local<v8::Object> obj;
    if (obj_defn->GetObjType() == FXJSOBJTYPE_GLOBAL) {
      obj = context->Global();
    } else if (!static_objects_[i].IsEmpty()) {
      obj = v8::Local<v8::Object>::New(GetIsolate(), static_objects_[i]);
      static_objects_[i].Reset();
    }
    if (!obj.IsEmpty()) {
      obj_defn->RunDestructor(obj);
      FreePerObjectData(obj);
    }
  }

  v8_context_.Reset();

  if (isolate_data->DecrementEngineRefCount() > 0) {
    return;
  }

  delete isolate_data;
  GetIsolate()->SetData(g_embedderDataSlot, nullptr);
}

std::optional<IJS_Runtime::JS_Error> CFXJS_Engine::Execute(
    const WideString& script) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::TryCatch try_catch(GetIsolate());
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  v8::Local<v8::Script> compiled_script;
  if (!v8::Script::Compile(context, NewString(script.AsStringView()))
           .ToLocal(&compiled_script)) {
    v8::String::Utf8Value error(GetIsolate(), try_catch.Exception());
    v8::Local<v8::Message> msg = try_catch.Message();
    auto [line, column] = GetLineAndColumnFromError(msg, context);
    return IJS_Runtime::JS_Error(line, column, WideString::FromUTF8(*error));
  }

  v8::Local<v8::Value> result;
  if (!compiled_script->Run(context).ToLocal(&result)) {
    v8::String::Utf8Value error(GetIsolate(), try_catch.Exception());
    auto msg = try_catch.Message();
    auto [line, column] = GetLineAndColumnFromError(msg, context);
    return IJS_Runtime::JS_Error(line, column, WideString::FromUTF8(*error));
  }
  return std::nullopt;
}

v8::Local<v8::Object> CFXJS_Engine::NewFXJSBoundObject(
    uint32_t obj_definition_id,
    FXJSOBJTYPE obj_type) {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  CFXJS_PerIsolateData* data = CFXJS_PerIsolateData::Get(GetIsolate());
  if (!data) {
    return v8::Local<v8::Object>();
  }

  CFXJS_ObjDefinition* obj_defn = data->ObjDefinitionForID(obj_definition_id);
  if (!obj_defn) {
    return v8::Local<v8::Object>();
  }

  v8::Local<v8::Object> obj;
  if (!obj_defn->GetInstanceTemplate()->NewInstance(context).ToLocal(&obj)) {
    return v8::Local<v8::Object>();
  }

  CFXJS_PerObjectData::SetNewDataInObject(obj_definition_id, obj);
  obj_defn->RunConstructor(this, obj);
  if (obj_type == FXJSOBJTYPE_DYNAMIC) {
    auto* isolate_data = CFXJS_PerIsolateData::Get(GetIsolate());
    V8TemplateMap* objs_map = isolate_data->GetDynamicObjsMap();
    if (objs_map) {
      objs_map->SetAndMakeWeak(obj);
    }
  }
  return obj;
}

v8::Local<v8::Object> CFXJS_Engine::GetThisObj() {
  v8::Isolate::Scope isolate_scope(GetIsolate());
  if (!CFXJS_PerIsolateData::Get(GetIsolate())) {
    return v8::Local<v8::Object>();
  }

  // Return the global object.
  v8::Local<v8::Context> context = GetIsolate()->GetCurrentContext();
  return context->Global();
}

void CFXJS_Engine::Error(const WideString& message) {
  fxv8::ThrowExceptionHelper(GetIsolate(), message.AsStringView());
}

v8::Local<v8::Context> CFXJS_Engine::GetV8Context() {
  return v8::Local<v8::Context>::New(GetIsolate(), v8_context_);
}

// static
CFXJS_PerObjectData::Binding* CFXJS_Engine::GetBinding(
    v8::Isolate* isolate,
    v8::Local<v8::Object> obj) {
  auto* data = CFXJS_PerObjectData::GetFromObject(obj);
  return data ? data->GetBinding() : nullptr;
}

v8::Local<v8::Array> CFXJS_Engine::GetConstArray(const WideString& name) {
  return v8::Local<v8::Array>::New(GetIsolate(), const_arrays_[name]);
}

void CFXJS_Engine::SetConstArray(const WideString& name,
                                 v8::Local<v8::Array> array) {
  const_arrays_[name] = v8::Global<v8::Array>(GetIsolate(), array);
}
