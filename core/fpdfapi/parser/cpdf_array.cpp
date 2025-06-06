// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_array.h"

#include <set>
#include <utility>

#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/notreached.h"

CPDF_Array::CPDF_Array() = default;

CPDF_Array::CPDF_Array(const WeakPtr<ByteStringPool>& pPool) : pool_(pPool) {}

CPDF_Array::~CPDF_Array() {
  // Break cycles for cyclic references.
  obj_num_ = kInvalidObjNum;
  for (auto& it : objects_) {
    if (it->GetObjNum() == kInvalidObjNum) {
      it.Leak();
    }
  }
}

CPDF_Object::Type CPDF_Array::GetType() const {
  return kArray;
}

CPDF_Array* CPDF_Array::AsMutableArray() {
  return this;
}

RetainPtr<CPDF_Object> CPDF_Array::Clone() const {
  return CloneObjectNonCyclic(false);
}

RetainPtr<CPDF_Object> CPDF_Array::CloneNonCyclic(
    bool bDirect,
    std::set<const CPDF_Object*>* pVisited) const {
  pVisited->insert(this);
  auto pCopy = pdfium::MakeRetain<CPDF_Array>();
  for (const auto& pValue : objects_) {
    if (!pdfium::Contains(*pVisited, pValue.Get())) {
      std::set<const CPDF_Object*> visited(*pVisited);
      if (auto obj = pValue->CloneNonCyclic(bDirect, &visited)) {
        pCopy->objects_.push_back(std::move(obj));
      }
    }
  }
  return pCopy;
}

CFX_FloatRect CPDF_Array::GetRect() const {
  CFX_FloatRect rect;
  if (objects_.size() != 4) {
    return rect;
  }

  rect.left = GetFloatAt(0);
  rect.bottom = GetFloatAt(1);
  rect.right = GetFloatAt(2);
  rect.top = GetFloatAt(3);
  return rect;
}

CFX_Matrix CPDF_Array::GetMatrix() const {
  if (objects_.size() != 6) {
    return CFX_Matrix();
  }

  return CFX_Matrix(GetFloatAt(0), GetFloatAt(1), GetFloatAt(2), GetFloatAt(3),
                    GetFloatAt(4), GetFloatAt(5));
}

std::optional<size_t> CPDF_Array::Find(const CPDF_Object* pThat) const {
  for (size_t i = 0; i < size(); ++i) {
    if (GetDirectObjectAt(i) == pThat) {
      return i;
    }
  }
  return std::nullopt;
}

bool CPDF_Array::Contains(const CPDF_Object* pThat) const {
  return Find(pThat).has_value();
}

CPDF_Object* CPDF_Array::GetMutableObjectAtInternal(size_t index) {
  return index < objects_.size() ? objects_[index].Get() : nullptr;
}

const CPDF_Object* CPDF_Array::GetObjectAtInternal(size_t index) const {
  return const_cast<CPDF_Array*>(this)->GetMutableObjectAtInternal(index);
}

RetainPtr<CPDF_Object> CPDF_Array::GetMutableObjectAt(size_t index) {
  return pdfium::WrapRetain(GetMutableObjectAtInternal(index));
}

RetainPtr<const CPDF_Object> CPDF_Array::GetObjectAt(size_t index) const {
  return pdfium::WrapRetain(GetObjectAtInternal(index));
}

RetainPtr<const CPDF_Object> CPDF_Array::GetDirectObjectAt(size_t index) const {
  return const_cast<CPDF_Array*>(this)->GetMutableDirectObjectAt(index);
}

RetainPtr<CPDF_Object> CPDF_Array::GetMutableDirectObjectAt(size_t index) {
  RetainPtr<CPDF_Object> pObj = GetMutableObjectAt(index);
  return pObj ? pObj->GetMutableDirect() : nullptr;
}

ByteString CPDF_Array::GetByteStringAt(size_t index) const {
  if (index >= objects_.size()) {
    return ByteString();
  }
  return objects_[index]->GetString();
}

WideString CPDF_Array::GetUnicodeTextAt(size_t index) const {
  if (index >= objects_.size()) {
    return WideString();
  }
  return objects_[index]->GetUnicodeText();
}

bool CPDF_Array::GetBooleanAt(size_t index, bool bDefault) const {
  if (index >= objects_.size()) {
    return bDefault;
  }
  const CPDF_Object* p = objects_[index].Get();
  return ToBoolean(p) ? p->GetInteger() != 0 : bDefault;
}

int CPDF_Array::GetIntegerAt(size_t index) const {
  if (index >= objects_.size()) {
    return 0;
  }
  return objects_[index]->GetInteger();
}

float CPDF_Array::GetFloatAt(size_t index) const {
  if (index >= objects_.size()) {
    return 0;
  }
  return objects_[index]->GetNumber();
}

RetainPtr<CPDF_Dictionary> CPDF_Array::GetMutableDictAt(size_t index) {
  RetainPtr<CPDF_Object> p = GetMutableDirectObjectAt(index);
  if (!p) {
    return nullptr;
  }
  CPDF_Dictionary* dict = p->AsMutableDictionary();
  if (dict) {
    return pdfium::WrapRetain(dict);
  }
  CPDF_Stream* pStream = p->AsMutableStream();
  if (pStream) {
    return pStream->GetMutableDict();
  }
  return nullptr;
}

RetainPtr<const CPDF_Dictionary> CPDF_Array::GetDictAt(size_t index) const {
  return const_cast<CPDF_Array*>(this)->GetMutableDictAt(index);
}

RetainPtr<CPDF_Stream> CPDF_Array::GetMutableStreamAt(size_t index) {
  return ToStream(GetMutableDirectObjectAt(index));
}

RetainPtr<const CPDF_Stream> CPDF_Array::GetStreamAt(size_t index) const {
  return const_cast<CPDF_Array*>(this)->GetMutableStreamAt(index);
}

RetainPtr<CPDF_Array> CPDF_Array::GetMutableArrayAt(size_t index) {
  return ToArray(GetMutableDirectObjectAt(index));
}

RetainPtr<const CPDF_Array> CPDF_Array::GetArrayAt(size_t index) const {
  return const_cast<CPDF_Array*>(this)->GetMutableArrayAt(index);
}

RetainPtr<const CPDF_Number> CPDF_Array::GetNumberAt(size_t index) const {
  return ToNumber(GetObjectAt(index));
}

RetainPtr<const CPDF_String> CPDF_Array::GetStringAt(size_t index) const {
  return ToString(GetObjectAt(index));
}

void CPDF_Array::Clear() {
  CHECK(!IsLocked());
  objects_.clear();
}

void CPDF_Array::RemoveAt(size_t index) {
  CHECK(!IsLocked());
  if (index < objects_.size()) {
    objects_.erase(objects_.begin() + index);
  }
}

void CPDF_Array::ConvertToIndirectObjectAt(size_t index,
                                           CPDF_IndirectObjectHolder* pHolder) {
  CHECK(!IsLocked());
  if (index >= objects_.size()) {
    return;
  }

  if (!objects_[index] || objects_[index]->IsReference()) {
    return;
  }

  pHolder->AddIndirectObject(objects_[index]);
  objects_[index] = objects_[index]->MakeReference(pHolder);
}

void CPDF_Array::SetAt(size_t index, RetainPtr<CPDF_Object> object) {
  (void)SetAtInternal(index, std::move(object));
}

void CPDF_Array::InsertAt(size_t index, RetainPtr<CPDF_Object> object) {
  (void)InsertAtInternal(index, std::move(object));
}

void CPDF_Array::Append(RetainPtr<CPDF_Object> object) {
  (void)AppendInternal(std::move(object));
}

CPDF_Object* CPDF_Array::SetAtInternal(size_t index,
                                       RetainPtr<CPDF_Object> pObj) {
  CHECK(!IsLocked());
  CHECK(pObj);
  CHECK(pObj->IsInline());
  CHECK(!pObj->IsStream());
  if (index >= objects_.size()) {
    return nullptr;
  }

  CPDF_Object* pRet = pObj.Get();
  objects_[index] = std::move(pObj);
  return pRet;
}

CPDF_Object* CPDF_Array::InsertAtInternal(size_t index,
                                          RetainPtr<CPDF_Object> pObj) {
  CHECK(!IsLocked());
  CHECK(pObj);
  CHECK(pObj->IsInline());
  CHECK(!pObj->IsStream());
  if (index > objects_.size()) {
    return nullptr;
  }

  CPDF_Object* pRet = pObj.Get();
  objects_.insert(objects_.begin() + index, std::move(pObj));
  return pRet;
}

CPDF_Object* CPDF_Array::AppendInternal(RetainPtr<CPDF_Object> pObj) {
  CHECK(!IsLocked());
  CHECK(pObj);
  CHECK(pObj->IsInline());
  CHECK(!pObj->IsStream());
  CPDF_Object* pRet = pObj.Get();
  objects_.push_back(std::move(pObj));
  return pRet;
}

bool CPDF_Array::WriteTo(IFX_ArchiveStream* archive,
                         const CPDF_Encryptor* encryptor) const {
  if (!archive->WriteString("[")) {
    return false;
  }

  for (size_t i = 0; i < size(); ++i) {
    if (!GetObjectAt(i)->WriteTo(archive, encryptor)) {
      return false;
    }
  }
  return archive->WriteString("]");
}

CPDF_ArrayLocker::CPDF_ArrayLocker(const CPDF_Array* pArray) : array_(pArray) {
  array_->lock_count_++;
}

CPDF_ArrayLocker::CPDF_ArrayLocker(RetainPtr<CPDF_Array> pArray)
    : array_(std::move(pArray)) {
  array_->lock_count_++;
}

CPDF_ArrayLocker::CPDF_ArrayLocker(RetainPtr<const CPDF_Array> pArray)
    : array_(std::move(pArray)) {
  array_->lock_count_++;
}

CPDF_ArrayLocker::~CPDF_ArrayLocker() {
  array_->lock_count_--;
}
