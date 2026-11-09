// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpdf_iconfit.h"

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fxcrt/fx_coordinates.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(CPDFIconFitTest, NullDict) {
  CPDF_IconFit icon_fit(nullptr);
  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kAlways, icon_fit.GetScaleMethod());
  EXPECT_TRUE(icon_fit.IsProportionalScale());
  EXPECT_FALSE(icon_fit.GetFittingBounds());
  EXPECT_EQ(CFX_PointF(0.5f, 0.5f), icon_fit.GetIconBottomLeftPosition());

  const CFX_SizeF image_size(100.0f, 50.0f);
  const CFX_FloatRect plate(0.0f, 0.0f, 200.0f, 200.0f);
  const CFX_VectorF scale = icon_fit.GetScale(image_size, plate);
  EXPECT_FLOAT_EQ(2.0f, scale.x);
  EXPECT_FLOAT_EQ(2.0f, scale.y);

  const CFX_VectorF offset = icon_fit.GetImageOffset(image_size, scale, plate);
  EXPECT_FLOAT_EQ(0.0f, offset.x);
  EXPECT_FLOAT_EQ(0.0f, offset.y);
}

TEST(CPDFIconFitTest, ScaleMethod) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  CPDF_IconFit icon_fit(dict);

  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kAlways, icon_fit.GetScaleMethod());

  dict->SetNewFor<CPDF_String>("SW", "A");
  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kAlways, icon_fit.GetScaleMethod());

  dict->SetNewFor<CPDF_String>("SW", "B");
  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kBigger, icon_fit.GetScaleMethod());

  dict->SetNewFor<CPDF_String>("SW", "S");
  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kSmaller, icon_fit.GetScaleMethod());

  dict->SetNewFor<CPDF_String>("SW", "N");
  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kNever, icon_fit.GetScaleMethod());

  dict->SetNewFor<CPDF_String>("SW", "unknown");
  EXPECT_EQ(CPDF_IconFit::ScaleMethod::kAlways, icon_fit.GetScaleMethod());
}

TEST(CPDFIconFitTest, IsProportionalScale) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  CPDF_IconFit icon_fit(dict);

  EXPECT_TRUE(icon_fit.IsProportionalScale());

  dict->SetNewFor<CPDF_String>("S", "P");
  EXPECT_TRUE(icon_fit.IsProportionalScale());

  dict->SetNewFor<CPDF_String>("S", "A");
  EXPECT_FALSE(icon_fit.IsProportionalScale());
}

TEST(CPDFIconFitTest, GetFittingBounds) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  CPDF_IconFit icon_fit(dict);

  EXPECT_FALSE(icon_fit.GetFittingBounds());

  dict->SetNewFor<CPDF_Boolean>("FB", true);
  EXPECT_TRUE(icon_fit.GetFittingBounds());

  dict->SetNewFor<CPDF_Boolean>("FB", false);
  EXPECT_FALSE(icon_fit.GetFittingBounds());
}

TEST(CPDFIconFitTest, GetIconBottomLeftPosition) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  CPDF_IconFit icon_fit(dict);

  EXPECT_EQ(CFX_PointF(0.5f, 0.5f), icon_fit.GetIconBottomLeftPosition());

  auto arr = dict->SetNewFor<CPDF_Array>("A");
  EXPECT_EQ(CFX_PointF(0.5f, 0.5f), icon_fit.GetIconBottomLeftPosition());

  arr->AppendNew<CPDF_Number>(0.25f);
  EXPECT_EQ(CFX_PointF(0.25f, 0.5f), icon_fit.GetIconBottomLeftPosition());

  arr->AppendNew<CPDF_Number>(0.75f);
  EXPECT_EQ(CFX_PointF(0.25f, 0.75f), icon_fit.GetIconBottomLeftPosition());
}

TEST(CPDFIconFitTest, GetScaleAlways) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  dict->SetNewFor<CPDF_String>("SW", "A");
  dict->SetNewFor<CPDF_String>("S", "A");  // Anamorphic.
  CPDF_IconFit icon_fit(dict);

  const CFX_SizeF image_size(100.0f, 50.0f);
  const CFX_FloatRect plate(0.0f, 0.0f, 300.0f, 200.0f);
  const CFX_VectorF scale_anamorphic = icon_fit.GetScale(image_size, plate);
  EXPECT_FLOAT_EQ(3.0f, scale_anamorphic.x);
  EXPECT_FLOAT_EQ(4.0f, scale_anamorphic.y);

  dict->SetNewFor<CPDF_String>("S", "P");  // Proportional.
  const CFX_VectorF scale_proportional = icon_fit.GetScale(image_size, plate);
  EXPECT_FLOAT_EQ(3.0f, scale_proportional.x);
  EXPECT_FLOAT_EQ(3.0f, scale_proportional.y);

  // Clamping for zero/negative image dimension.
  const CFX_VectorF scale_clamped =
      icon_fit.GetScale(CFX_SizeF(0.0f, 0.0f), plate);
  EXPECT_FLOAT_EQ(200.0f, scale_clamped.x);
  EXPECT_FLOAT_EQ(200.0f, scale_clamped.y);
}

TEST(CPDFIconFitTest, GetScaleBigger) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  dict->SetNewFor<CPDF_String>("SW", "B");
  dict->SetNewFor<CPDF_String>("S", "A");  // Anamorphic.
  CPDF_IconFit icon_fit(dict);

  // Plate smaller than image: scales down.
  const CFX_SizeF large_image(400.0f, 200.0f);
  const CFX_FloatRect small_plate(0.0f, 0.0f, 200.0f, 50.0f);
  const CFX_VectorF scale_down = icon_fit.GetScale(large_image, small_plate);
  EXPECT_FLOAT_EQ(0.5f, scale_down.x);
  EXPECT_FLOAT_EQ(0.25f, scale_down.y);

  // Plate larger than image: remains 1.0f.
  const CFX_FloatRect large_plate(0.0f, 0.0f, 800.0f, 400.0f);
  const CFX_VectorF scale_none = icon_fit.GetScale(large_image, large_plate);
  EXPECT_FLOAT_EQ(1.0f, scale_none.x);
  EXPECT_FLOAT_EQ(1.0f, scale_none.y);
}

TEST(CPDFIconFitTest, GetScaleSmaller) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  dict->SetNewFor<CPDF_String>("SW", "S");
  dict->SetNewFor<CPDF_String>("S", "A");  // Anamorphic.
  CPDF_IconFit icon_fit(dict);

  // Plate larger than image: scales up.
  const CFX_SizeF small_image(50.0f, 100.0f);
  const CFX_FloatRect large_plate(0.0f, 0.0f, 200.0f, 300.0f);
  const CFX_VectorF scale_up = icon_fit.GetScale(small_image, large_plate);
  EXPECT_FLOAT_EQ(4.0f, scale_up.x);
  EXPECT_FLOAT_EQ(3.0f, scale_up.y);

  // Plate smaller than image: remains 1.0f.
  const CFX_FloatRect small_plate(0.0f, 0.0f, 25.0f, 50.0f);
  const CFX_VectorF scale_none = icon_fit.GetScale(small_image, small_plate);
  EXPECT_FLOAT_EQ(1.0f, scale_none.x);
  EXPECT_FLOAT_EQ(1.0f, scale_none.y);
}

TEST(CPDFIconFitTest, GetScaleNever) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  dict->SetNewFor<CPDF_String>("SW", "N");
  CPDF_IconFit icon_fit(dict);

  const CFX_SizeF image_size(100.0f, 50.0f);
  const CFX_FloatRect plate(0.0f, 0.0f, 500.0f, 500.0f);
  const CFX_VectorF scale = icon_fit.GetScale(image_size, plate);
  EXPECT_FLOAT_EQ(1.0f, scale.x);
  EXPECT_FLOAT_EQ(1.0f, scale.y);
}

TEST(CPDFIconFitTest, GetImageOffset) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  auto arr = dict->SetNewFor<CPDF_Array>("A");
  arr->AppendNew<CPDF_Number>(0.5f);
  arr->AppendNew<CPDF_Number>(0.5f);
  CPDF_IconFit icon_fit(dict);

  const CFX_SizeF image_size(100.0f, 50.0f);
  const CFX_VectorF scale(1.0f, 1.0f);
  const CFX_FloatRect plate(0.0f, 0.0f, 200.0f, 200.0f);
  const CFX_VectorF offset = icon_fit.GetImageOffset(image_size, scale, plate);
  // (200 - 100) * 0.5 = 50
  // (200 - 50) * 0.5 = 75
  EXPECT_FLOAT_EQ(50.0f, offset.x);
  EXPECT_FLOAT_EQ(75.0f, offset.y);
}
