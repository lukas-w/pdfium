// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xfa/fgas/crt/cfgas_decimal.h"

#include <math.h>

#include <limits>

#include "testing/gtest/include/gtest/gtest.h"

TEST(CFGASDecimalTest, Empty) {
  CFGAS_Decimal empty;
  EXPECT_EQ(L"0", empty.ToWideString());
  EXPECT_EQ(0.0f, empty.ToFloat());
  EXPECT_EQ(0.0, empty.ToDouble());
}

TEST(CFGASDecimalTest, FromInt32) {
  CFGAS_Decimal big(std::numeric_limits<int32_t>::max());
  EXPECT_EQ(L"2147483647", big.ToWideString());

  CFGAS_Decimal small(std::numeric_limits<int32_t>::min());
  EXPECT_EQ(L"-2147483648", small.ToWideString());

  CFGAS_Decimal neg(-12345);
  EXPECT_EQ(L"-12345", neg.ToWideString());

  CFGAS_Decimal pos(12345);
  EXPECT_EQ(L"12345", pos.ToWideString());

  CFGAS_Decimal zero(0);
  EXPECT_EQ(L"0", zero.ToWideString());
}

TEST(CFGASDecimalTest, FromUint32) {
  CFGAS_Decimal big(std::numeric_limits<uint32_t>::max());
  CFGAS_Decimal small(std::numeric_limits<uint32_t>::min());
  EXPECT_EQ(L"4294967295", big.ToWideString());
  EXPECT_EQ(L"0", small.ToWideString());
}

TEST(CFGASDecimalTest, FromUint64) {
  CFGAS_Decimal big(std::numeric_limits<uint64_t>::max());
  CFGAS_Decimal small(std::numeric_limits<uint64_t>::min());
  EXPECT_EQ(L"18446744073709551615", big.ToWideString());
  EXPECT_EQ(L"0", small.ToWideString());
}

TEST(CFGASDecimalTest, FromFloat) {
  WideString big = CFGAS_Decimal(powf(2.0f, 95.0f), 0).ToWideString();
  WideString big_expected = L"39614081257132168796771975168";

  // Precision may not be the same on all platforms.
  EXPECT_EQ(big_expected.GetLength(), big.GetLength());
  EXPECT_EQ(big_expected.First(8).c_str(), big.First(8));

  WideString tiny = CFGAS_Decimal(1e20f, 0).ToWideString();
  WideString tiny_expected = L"100000000000000000000";
  EXPECT_EQ(tiny_expected.GetLength(), tiny.GetLength());
  EXPECT_EQ(tiny_expected.First(8).c_str(), tiny.First(8));

  WideString teeny = CFGAS_Decimal(1e14f, 4).ToWideString();
  WideString teeny_expected = L"100000000000000.0000";
  EXPECT_EQ(teeny_expected.GetLength(), teeny.GetLength());
  EXPECT_EQ(teeny_expected.First(8).c_str(), teeny.First(8));
}

TEST(CFGASDecimalTest, FromFloatFractional) {
  WideString case1 = CFGAS_Decimal(123.456f, 10).ToWideString();
  WideString case1_expected = L"123.4560000000";

  // Precision may not be the same on all platforms.
  EXPECT_EQ(case1_expected.GetLength(), case1.GetLength());
  EXPECT_EQ(case1_expected.First(8).c_str(), case1.First(8));
}

TEST(CFGASDecimalTest, FromString) {
  CFGAS_Decimal big(L"100000000000000000000000000");
  EXPECT_EQ(L"100000000000000000000000000", big.ToWideString());

  CFGAS_Decimal small(L"-1000000000000000000000000");
  EXPECT_EQ(L"-1000000000000000000000000", small.ToWideString());

  CFGAS_Decimal leading_spaces(L"   456");
  EXPECT_EQ(L"456", leading_spaces.ToWideString());

  CFGAS_Decimal leading_plus(L"+789");
  EXPECT_EQ(L"789", leading_plus.ToWideString());

  CFGAS_Decimal spaces_plus(L"   +123.45");
  EXPECT_EQ(L"123.45", spaces_plus.ToWideString());

  CFGAS_Decimal spaces_minus(L"   -123.45");
  EXPECT_EQ(L"-123.45", spaces_minus.ToWideString());

  CFGAS_Decimal leading_dot(L".5");
  EXPECT_EQ(L"0.5", leading_dot.ToWideString());

  CFGAS_Decimal trailing_dot(L"42.");
  EXPECT_EQ(L"42", trailing_dot.ToWideString());

  CFGAS_Decimal non_digit_term(L"123abc456");
  EXPECT_EQ(L"123", non_digit_term.ToWideString());

  CFGAS_Decimal empty_str(L"");
  EXPECT_EQ(L"0", empty_str.ToWideString());

  CFGAS_Decimal blank_str(L"   ");
  EXPECT_EQ(L"0", blank_str.ToWideString());
}

TEST(CFGASDecimalTest, FromString28Digits) {
  CFGAS_Decimal frac(L"32109876543210.0123456890123");
  EXPECT_EQ(L"32109876543210.0123456890123", frac.ToWideString());
}

TEST(CFGASDecimalTest, ScaleAndNegate) {
  CFGAS_Decimal val(L"12.34");
  EXPECT_TRUE(val.IsNotZero());
  EXPECT_EQ(2, val.GetScale());

  // Same scale is a no-op.
  val.SetScale(2);
  EXPECT_EQ(2, val.GetScale());
  EXPECT_EQ(L"12.34", val.ToWideString());

  // Increase scale.
  val.SetScale(4);
  EXPECT_EQ(4, val.GetScale());
  EXPECT_EQ(L"12.3400", val.ToWideString());

  // Decrease scale with rounding.
  val.SetScale(1);
  EXPECT_EQ(1, val.GetScale());
  EXPECT_EQ(L"12.3", val.ToWideString());

  val.SetScale(0);
  EXPECT_EQ(0, val.GetScale());
  EXPECT_EQ(L"12", val.ToWideString());

  // Negate non-zero.
  val.SetNegate();
  EXPECT_EQ(L"-12", val.ToWideString());
  val.SetNegate();
  EXPECT_EQ(L"12", val.ToWideString());

  // Negate zero.
  CFGAS_Decimal zero;
  EXPECT_FALSE(zero.IsNotZero());
  zero.SetNegate();
  EXPECT_FALSE(zero.IsNotZero());
  EXPECT_EQ(L"0", zero.ToWideString());
}

TEST(CFGASDecimalTest, FloatDoubleConversion) {
  CFGAS_Decimal int_val(123);
  EXPECT_FLOAT_EQ(123.0f, int_val.ToFloat());
  EXPECT_DOUBLE_EQ(123.0, int_val.ToDouble());

  CFGAS_Decimal pos(L"123.5");
  EXPECT_FLOAT_EQ(123.5f, pos.ToFloat());
  EXPECT_DOUBLE_EQ(123.5, pos.ToDouble());

  CFGAS_Decimal neg(L"-123.5");
  EXPECT_FLOAT_EQ(-123.5f, neg.ToFloat());
  EXPECT_DOUBLE_EQ(-123.5, neg.ToDouble());
}

TEST(CFGASDecimalTest, Multiply) {
  CFGAS_Decimal zero;
  CFGAS_Decimal num(L"42");
  EXPECT_EQ(L"0", (zero * num).ToWideString());
  EXPECT_EQ(L"0", (num * zero).ToWideString());

  CFGAS_Decimal a(12);
  CFGAS_Decimal b(34);
  EXPECT_EQ(L"408", (a * b).ToWideString());

  CFGAS_Decimal c(-12);
  EXPECT_EQ(L"-408", (c * b).ToWideString());
  EXPECT_EQ(L"408", (c * CFGAS_Decimal(-34)).ToWideString());

  CFGAS_Decimal frac1(L"1.5");
  CFGAS_Decimal frac2(L"2.5");
  EXPECT_EQ(L"3.75", (frac1 * frac2).ToWideString());

  // Multiplying numbers with high precision triggers scale shrinking.
  CFGAS_Decimal hi1(L"12345678901234.12345678901234");
  CFGAS_Decimal hi2(L"98765432109876.54321098765432");
  CFGAS_Decimal product = hi1 * hi2;
  EXPECT_TRUE(product.IsNotZero());
  EXPECT_LE(product.GetScale(), 28);
}

TEST(CFGASDecimalTest, DivBy10AndDivBy100) {
  CFGAS_Decimal zero;
  zero.DivBy10();
  EXPECT_EQ(L"0", zero.ToWideString());
  zero.DivBy100();
  EXPECT_EQ(L"0", zero.ToWideString());

  CFGAS_Decimal a(100);
  a.DivBy10();
  EXPECT_EQ(L"10", a.ToWideString());
  a.DivBy10();
  EXPECT_EQ(L"1", a.ToWideString());
  a.DivBy10();
  EXPECT_EQ(L"0.1", a.ToWideString());

  CFGAS_Decimal b(L"10.0");
  b.DivBy10();
  EXPECT_EQ(L"1.0", b.ToWideString());

  CFGAS_Decimal c(123);
  c.DivBy10();
  EXPECT_EQ(L"12.3", c.ToWideString());

  CFGAS_Decimal neg(-100);
  neg.DivBy10();
  EXPECT_EQ(L"-10", neg.ToWideString());
  neg.DivBy10();
  EXPECT_EQ(L"-1", neg.ToWideString());

  CFGAS_Decimal pct(50);
  pct.DivBy100();
  EXPECT_EQ(L"0.5", pct.ToWideString());

  CFGAS_Decimal pct100(100);
  pct100.DivBy100();
  EXPECT_EQ(L"1", pct100.ToWideString());

  CFGAS_Decimal pct_neg(-75);
  pct_neg.DivBy100();
  EXPECT_EQ(L"-0.75", pct_neg.ToWideString());
}
