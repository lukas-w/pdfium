// Copyright 2018 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/render/cpdf_docrenderdata.h"

#include <iterator>
#include <memory>
#include <utility>

#include "core/fpdfapi/page/cpdf_transferfunc.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_indirect_object_holder.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fxcrt/data_vector.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::ElementsAreArray;

namespace {

constexpr uint8_t kExpectedType0FunctionSamples[] = {
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
    26, 26, 26, 26, 26, 26, 26, 26, 26};

constexpr uint8_t kExpectedType2FunctionSamples[] = {
    26, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25,
    25, 25, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 21, 21, 21, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
    19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 18, 18, 18, 18, 18, 18, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,
    17, 17, 17, 17, 17, 17, 17, 17, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13};

constexpr uint8_t kExpectedType4FunctionSamples[] = {
    0,   3,   6,   9,   13,  16,  19,  22,  25,  28,  31,  34,  37,  40,  43,
    46,  49,  52,  55,  58,  60,  63,  66,  68,  71,  74,  76,  79,  81,  84,
    86,  88,  90,  93,  95,  97,  99,  101, 103, 105, 106, 108, 110, 111, 113,
    114, 115, 117, 118, 119, 120, 121, 122, 123, 124, 125, 125, 126, 126, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 126, 126, 125, 125, 124, 123,
    123, 122, 121, 120, 119, 117, 116, 115, 113, 112, 110, 109, 107, 105, 104,
    102, 100, 98,  96,  94,  92,  89,  87,  85,  82,  80,  77,  75,  72,  70,
    67,  64,  62,  59,  56,  53,  50,  48,  45,  42,  39,  36,  33,  30,  27,
    23,  20,  17,  14,  11,  8,   5,   2,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0};

RetainPtr<CPDF_Reference> CreateType0FunctionStreamReference(
    CPDF_IndirectObjectHolder& holder) {
  auto func_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  func_dict->SetNewFor<CPDF_Number>("FunctionType", 0);
  func_dict->SetNewFor<CPDF_Number>("BitsPerSample", 8);

  auto domain_array = func_dict->SetNewFor<CPDF_Array>("Domain");
  domain_array->AppendNew<CPDF_Number>(0);
  domain_array->AppendNew<CPDF_Number>(1);

  auto range_array = func_dict->SetNewFor<CPDF_Array>("Range");
  range_array->AppendNew<CPDF_Number>(0);
  range_array->AppendNew<CPDF_Number>(0.5f);

  auto size_array = func_dict->SetNewFor<CPDF_Array>("Size");
  size_array->AppendNew<CPDF_Number>(4);

  static constexpr uint8_t kContents[] = "1234";
  auto stream = holder.NewIndirect<CPDF_Stream>(
      DataVector<uint8_t>(std::begin(kContents), std::end(kContents)),
      std::move(func_dict));
  return pdfium::MakeRetain<CPDF_Reference>(&holder, stream->GetObjNum());
}

RetainPtr<CPDF_Dictionary> CreateType2FunctionDict() {
  auto func_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  func_dict->SetNewFor<CPDF_Number>("FunctionType", 2);
  func_dict->SetNewFor<CPDF_Number>("N", 1);

  auto domain_array = func_dict->SetNewFor<CPDF_Array>("Domain");
  domain_array->AppendNew<CPDF_Number>(0);
  domain_array->AppendNew<CPDF_Number>(1);

  auto c0_array = func_dict->SetNewFor<CPDF_Array>("C0");
  c0_array->AppendNew<CPDF_Number>(0.1f);
  c0_array->AppendNew<CPDF_Number>(0.2f);
  c0_array->AppendNew<CPDF_Number>(0.8f);

  auto c1_array = func_dict->SetNewFor<CPDF_Array>("C1");
  c1_array->AppendNew<CPDF_Number>(0.05f);
  c1_array->AppendNew<CPDF_Number>(0.01f);
  c1_array->AppendNew<CPDF_Number>(0.4f);

  return func_dict;
}

RetainPtr<CPDF_Reference> CreateType4FunctionStreamReference(
    CPDF_IndirectObjectHolder& holder) {
  auto func_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  func_dict->SetNewFor<CPDF_Number>("FunctionType", 4);

  auto domain_array = func_dict->SetNewFor<CPDF_Array>("Domain");
  domain_array->AppendNew<CPDF_Number>(0);
  domain_array->AppendNew<CPDF_Number>(1);

  auto range_array = func_dict->SetNewFor<CPDF_Array>("Range");
  range_array->AppendNew<CPDF_Number>(-1);
  range_array->AppendNew<CPDF_Number>(1);

  static constexpr uint8_t kContents[] = "{ 360 mul sin 2 div }";
  auto stream = holder.NewIndirect<CPDF_Stream>(
      DataVector<uint8_t>(std::begin(kContents), std::end(kContents)),
      std::move(func_dict));
  return pdfium::MakeRetain<CPDF_Reference>(&holder, stream->GetObjNum());
}

RetainPtr<CPDF_Stream> CreateBadType4FunctionStream() {
  auto func_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  func_dict->SetNewFor<CPDF_Number>("FunctionType", 4);

  auto domain_array = func_dict->SetNewFor<CPDF_Array>("Domain");
  domain_array->AppendNew<CPDF_Number>(0);
  domain_array->AppendNew<CPDF_Number>(1);

  auto range_array = func_dict->SetNewFor<CPDF_Array>("Range");
  range_array->AppendNew<CPDF_Number>(-1);
  range_array->AppendNew<CPDF_Number>(1);

  static constexpr uint8_t kContents[] = "garbage";
  return pdfium::MakeRetain<CPDF_Stream>(
      DataVector<uint8_t>(std::begin(kContents), std::end(kContents)),
      std::move(func_dict));
}

class TestDocRenderData : public CPDF_DocRenderData {
 public:
  TestDocRenderData() = default;

  RetainPtr<CPDF_TransferFunc> CreateTransferFuncForTesting(
      RetainPtr<const CPDF_Object> pObj) const {
    return CreateTransferFunc(std::move(pObj));
  }
};

TEST(CPDFDocRenderDataTest, TransferFunctionOne) {
  RetainPtr<CPDF_Dictionary> func_dict = CreateType2FunctionDict();

  TestDocRenderData render_data;
  auto func = render_data.CreateTransferFuncForTesting(func_dict);
  ASSERT_TRUE(func);
  EXPECT_FALSE(func->GetIdentity());
  EXPECT_THAT(func->GetSamplesR(),
              ElementsAreArray(kExpectedType2FunctionSamples));
  EXPECT_THAT(func->GetSamplesG(),
              ElementsAreArray(kExpectedType2FunctionSamples));
  EXPECT_THAT(func->GetSamplesB(),
              ElementsAreArray(kExpectedType2FunctionSamples));
  EXPECT_EQ(0x000d0d0du, func->TranslateColor(0x00ffffff));
  EXPECT_EQ(0x000d1a1au, func->TranslateColor(0x00ff0000));
  EXPECT_EQ(0x001a0d1au, func->TranslateColor(0x0000ff00));
  EXPECT_EQ(0x001a1a0du, func->TranslateColor(0x000000ff));
  EXPECT_EQ(0x000f0f0fu, func->TranslateColor(0x00cccccc));
  EXPECT_EQ(0x00191715u, func->TranslateColor(0x00123456));
  EXPECT_EQ(0x000d0d0du, func->TranslateColor(0xffffffff));
  EXPECT_EQ(0x001a1a1au, func->TranslateColor(0xff000000));
  EXPECT_EQ(0x000d0d0du, func->TranslateColor(0xccffffff));
  EXPECT_EQ(0x001a1a1au, func->TranslateColor(0x99000000));
}

TEST(CPDFDocRenderDataTest, TransferFunctionArray) {
  CPDF_IndirectObjectHolder holder;
  auto func_array = pdfium::MakeRetain<CPDF_Array>();
  func_array->Append(CreateType0FunctionStreamReference(holder));
  func_array->Append(CreateType2FunctionDict());
  func_array->Append(CreateType4FunctionStreamReference(holder));

  TestDocRenderData render_data;
  auto func = render_data.CreateTransferFuncForTesting(func_array);
  ASSERT_TRUE(func);
  EXPECT_FALSE(func->GetIdentity());
  EXPECT_THAT(func->GetSamplesR(),
              ElementsAreArray(kExpectedType0FunctionSamples));
  EXPECT_THAT(func->GetSamplesG(),
              ElementsAreArray(kExpectedType2FunctionSamples));
  EXPECT_THAT(func->GetSamplesB(),
              ElementsAreArray(kExpectedType4FunctionSamples));
  EXPECT_EQ(0x00000d1au, func->TranslateColor(0x00ffffff));
  EXPECT_EQ(0x00001a19u, func->TranslateColor(0x00ff0000));
  EXPECT_EQ(0x00000d19u, func->TranslateColor(0x0000ff00));
  EXPECT_EQ(0x00001a1au, func->TranslateColor(0x000000ff));
  EXPECT_EQ(0x00000f1au, func->TranslateColor(0x00cccccc));
  EXPECT_EQ(0x00371719u, func->TranslateColor(0x00123456));
  EXPECT_EQ(0x00000d1au, func->TranslateColor(0xffffffff));
  EXPECT_EQ(0x00001a19u, func->TranslateColor(0xff000000));
  EXPECT_EQ(0x00000d1au, func->TranslateColor(0xccffffff));
  EXPECT_EQ(0x00001a19u, func->TranslateColor(0x99000000));
}

TEST(CPDFDocRenderDataTest, TransferFunctionClamping) {
  // Type 2 exponential function: f(x) = C0 + x^N * (C1 - C0).
  // With C0 = -1.0, C1 = 2.0, N = 1:
  // f(0.0) = -1.0 (negative, underflow)
  // f(0.5) = 0.5
  // f(1.0) = 2.0 (greater than 1.0, overflow)
  auto func_dict = pdfium::MakeRetain<CPDF_Dictionary>();
  func_dict->SetNewFor<CPDF_Number>("FunctionType", 2);
  func_dict->SetNewFor<CPDF_Number>("N", 1);

  auto domain_array = func_dict->SetNewFor<CPDF_Array>("Domain");
  domain_array->AppendNew<CPDF_Number>(0);
  domain_array->AppendNew<CPDF_Number>(1);

  auto c0_array = func_dict->SetNewFor<CPDF_Array>("C0");
  c0_array->AppendNew<CPDF_Number>(-1.0f);

  auto c1_array = func_dict->SetNewFor<CPDF_Array>("C1");
  c1_array->AppendNew<CPDF_Number>(2.0f);

  TestDocRenderData render_data;
  auto func = render_data.CreateTransferFuncForTesting(func_dict);
  ASSERT_TRUE(func);
  EXPECT_FALSE(func->GetIdentity());

  // Input 0 gives -1.0f, clamped to 0.
  EXPECT_EQ(0, func->GetSamplesR()[0]);
  EXPECT_EQ(0, func->GetSamplesG()[0]);
  EXPECT_EQ(0, func->GetSamplesB()[0]);

  // Input 128 gives 0.505882f -> 129.
  EXPECT_EQ(129, func->GetSamplesR()[128]);
  EXPECT_EQ(129, func->GetSamplesG()[128]);
  EXPECT_EQ(129, func->GetSamplesB()[128]);

  // Input 255 gives 2.0f, clamped to 255.
  EXPECT_EQ(255, func->GetSamplesR()[255]);
  EXPECT_EQ(255, func->GetSamplesG()[255]);
  EXPECT_EQ(255, func->GetSamplesB()[255]);
}

TEST(CPDFDocRenderDataTest, BadTransferFunctions) {
  {
    auto func_stream = CreateBadType4FunctionStream();

    TestDocRenderData render_data;
    auto func = render_data.CreateTransferFuncForTesting(func_stream);
    EXPECT_FALSE(func);
  }

  {
    auto func_array = pdfium::MakeRetain<CPDF_Array>();

    TestDocRenderData render_data;
    auto func = render_data.CreateTransferFuncForTesting(func_array);
    EXPECT_FALSE(func);
  }

  {
    CPDF_IndirectObjectHolder holder;
    auto func_array = pdfium::MakeRetain<CPDF_Array>();
    func_array->Append(CreateType0FunctionStreamReference(holder));
    func_array->Append(CreateType2FunctionDict());
    auto func_stream = CreateBadType4FunctionStream();
    const int func_stream_object_number =
        holder.AddIndirectObject(std::move(func_stream));
    func_array->Append(
        pdfium::MakeRetain<CPDF_Reference>(&holder, func_stream_object_number));

    TestDocRenderData render_data;
    auto func = render_data.CreateTransferFuncForTesting(func_array);
    EXPECT_FALSE(func);
  }
}

}  // namespace
