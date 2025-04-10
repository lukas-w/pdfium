// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/parser/cpdf_object_avail.h"

#include <map>
#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_indirect_object_holder.h"
#include "core/fpdfapi/parser/cpdf_read_validator.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/notreached.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/invalid_seekable_read_stream.h"

namespace {

class TestReadValidator final : public CPDF_ReadValidator {
 public:
  CONSTRUCT_VIA_MAKE_RETAIN;

  void SimulateReadError() { ReadBlockAtOffset({}, 0); }

 private:
  TestReadValidator()
      : CPDF_ReadValidator(pdfium::MakeRetain<InvalidSeekableReadStream>(100),
                           nullptr) {}
  ~TestReadValidator() override = default;
};

class TestHolder final : public CPDF_IndirectObjectHolder {
 public:
  enum class ObjectState {
    Unavailable,
    Available,
  };
  TestHolder() : validator_(pdfium::MakeRetain<TestReadValidator>()) {}
  ~TestHolder() override = default;

  // CPDF_IndirectObjectHolder overrides:
  RetainPtr<CPDF_Object> ParseIndirectObject(uint32_t objnum) override {
    auto it = objects_data_.find(objnum);
    if (it == objects_data_.end()) {
      return nullptr;
    }

    ObjectData& obj_data = it->second;
    if (obj_data.state == ObjectState::Unavailable) {
      validator_->SimulateReadError();
      return nullptr;
    }
    return obj_data.object;
  }

  RetainPtr<CPDF_ReadValidator> GetValidator() { return validator_; }

  void AddObject(uint32_t objnum,
                 RetainPtr<CPDF_Object> object,
                 ObjectState state) {
    ObjectData object_data;
    object_data.object = std::move(object);
    object_data.state = state;
    DCHECK(objects_data_.find(objnum) == objects_data_.end());
    objects_data_[objnum] = std::move(object_data);
  }

  void SetObjectState(uint32_t objnum, ObjectState state) {
    auto it = objects_data_.find(objnum);
    DCHECK(it != objects_data_.end());
    ObjectData& obj_data = it->second;
    obj_data.state = state;
  }

  CPDF_Object* GetTestObject(uint32_t objnum) {
    auto it = objects_data_.find(objnum);
    if (it == objects_data_.end()) {
      return nullptr;
    }
    return it->second.object.Get();
  }

 private:
  struct ObjectData {
    RetainPtr<CPDF_Object> object;
    ObjectState state = ObjectState::Unavailable;
  };
  std::map<uint32_t, ObjectData> objects_data_;
  RetainPtr<TestReadValidator> validator_;
};

class CPDF_ObjectAvailFailOnExclude final : public CPDF_ObjectAvail {
 public:
  using CPDF_ObjectAvail::CPDF_ObjectAvail;
  ~CPDF_ObjectAvailFailOnExclude() override = default;
  bool ExcludeObject(const CPDF_Object* object) const override { NOTREACHED(); }
};

class CPDF_ObjectAvailExcludeArray final : public CPDF_ObjectAvail {
 public:
  using CPDF_ObjectAvail::CPDF_ObjectAvail;
  ~CPDF_ObjectAvailExcludeArray() override = default;
  bool ExcludeObject(const CPDF_Object* object) const override {
    return object->IsArray();
  }
};

class CPDF_ObjectAvailExcludeTypeKey final : public CPDF_ObjectAvail {
 public:
  using CPDF_ObjectAvail::CPDF_ObjectAvail;
  ~CPDF_ObjectAvailExcludeTypeKey() override = default;
  bool ExcludeObject(const CPDF_Object* object) const override {
    // The value of "Type" may be reference, and if it is not available, we can
    // incorrect filter objects.
    // In this case CPDF_ObjectAvail should wait availability of this item and
    // call ExcludeObject again.
    return object->IsDictionary() &&
           object->GetDict()->GetByteStringFor("Type") == "Exclude me";
  }
};

}  // namespace

TEST(ObjectAvailTest, OneObject) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_String>(nullptr, "string"),
                   TestHolder::ObjectState::Unavailable);
  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());
  holder.SetObjectState(1, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, OneReferencedObject) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Reference>(&holder, 2),
                   TestHolder::ObjectState::Unavailable);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_String>(nullptr, "string"),
                   TestHolder::ObjectState::Unavailable);
  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(1, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(2, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, CycledReferences) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Reference>(&holder, 2),
                   TestHolder::ObjectState::Unavailable);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_Reference>(&holder, 3),
                   TestHolder::ObjectState::Unavailable);
  holder.AddObject(3, pdfium::MakeRetain<CPDF_Reference>(&holder, 1),
                   TestHolder::ObjectState::Unavailable);

  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(1, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(2, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(3, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, DoNotCheckParent) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Unavailable);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Unavailable);

  holder.GetTestObject(2)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "Parent", &holder, 1);

  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 2);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(2, TestHolder::ObjectState::Available);
  //  Object should be available in case when "Parent" object is unavailable.
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, Generic) {
  TestHolder holder;
  const uint32_t kDepth = 100;
  for (uint32_t i = 1; i < kDepth; ++i) {
    holder.AddObject(i, pdfium::MakeRetain<CPDF_Dictionary>(),
                     TestHolder::ObjectState::Unavailable);
    // Add ref to next dictionary.
    holder.GetTestObject(i)->GetMutableDict()->SetNewFor<CPDF_Reference>(
        "Child", &holder, i + 1);
  }
  // Add final object
  holder.AddObject(kDepth, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Unavailable);

  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 1);
  for (uint32_t i = 1; i <= kDepth; ++i) {
    EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());
    holder.SetObjectState(i, TestHolder::ObjectState::Available);
  }
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, NotExcludeRoot) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  CPDF_ObjectAvailFailOnExclude avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, NotExcludeReferedRoot) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Reference>(&holder, 2),
                   TestHolder::ObjectState::Available);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  CPDF_ObjectAvailFailOnExclude avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, Exclude) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(1)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "ArrayRef", &holder, 2);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_Array>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(2)->AsMutableArray()->AppendNew<CPDF_Reference>(&holder,
                                                                       2);

  // Add string, which is refered by array item. It is should not be checked.
  holder.AddObject(
      3, pdfium::MakeRetain<CPDF_String>(nullptr, "Not available string"),
      TestHolder::ObjectState::Unavailable);
  CPDF_ObjectAvailExcludeArray avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, ReadErrorOnExclude) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(1)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "DictRef", &holder, 2);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);

  holder.GetTestObject(2)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "Type", &holder, 3);
  // The value of "Type" key is not available at start
  holder.AddObject(3, pdfium::MakeRetain<CPDF_String>(nullptr, "Exclude me"),
                   TestHolder::ObjectState::Unavailable);

  holder.GetTestObject(2)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "OtherData", &holder, 4);
  // Add string, which is referred by dictionary item. It is should not be
  // checked, because the dictionary with it, should be skipped.
  holder.AddObject(
      4, pdfium::MakeRetain<CPDF_String>(nullptr, "Not available string"),
      TestHolder::ObjectState::Unavailable);

  CPDF_ObjectAvailExcludeTypeKey avail(holder.GetValidator(), &holder, 1);

  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  // Make "Type" value object available.
  holder.SetObjectState(3, TestHolder::ObjectState::Available);

  // Now object should be available, although the object '4' is not available,
  // because it is in skipped dictionary.
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, IgnoreNotExistsObject) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);
  holder.GetTestObject(1)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "NotExistsObjRef", &holder, 2);
  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 1);
  // Now object should be available, although the object '2' is not exists. But
  // all exists in file related data are checked.
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}

TEST(ObjectAvailTest, CheckTwice) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_String>(nullptr, "string"),
                   TestHolder::ObjectState::Unavailable);

  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, 1);
  EXPECT_EQ(avail.CheckAvail(), avail.CheckAvail());

  holder.SetObjectState(1, TestHolder::ObjectState::Available);
  EXPECT_EQ(avail.CheckAvail(), avail.CheckAvail());
}

TEST(ObjectAvailTest, SelfReferedInlinedObject) {
  TestHolder holder;
  holder.AddObject(1, pdfium::MakeRetain<CPDF_Dictionary>(),
                   TestHolder::ObjectState::Available);

  holder.GetTestObject(1)->GetMutableDict()->SetNewFor<CPDF_Reference>(
      "Data", &holder, 2);
  auto root =
      holder.GetTestObject(1)->GetMutableDict()->SetNewFor<CPDF_Dictionary>(
          "Dict");

  root->SetNewFor<CPDF_Reference>("Self", &holder, 1);
  holder.AddObject(2, pdfium::MakeRetain<CPDF_String>(nullptr, "Data"),
                   TestHolder::ObjectState::Unavailable);

  CPDF_ObjectAvail avail(holder.GetValidator(), &holder, root);
  EXPECT_EQ(CPDF_DataAvail::kDataNotAvailable, avail.CheckAvail());

  holder.SetObjectState(2, TestHolder::ObjectState::Available);
  EXPECT_EQ(CPDF_DataAvail::kDataAvailable, avail.CheckAvail());
}
