// Copyright 2021 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_datetime.h"

#include "core/fxcrt/fake_time_test.h"

TEST_F(FakeTimeTest, Now) {
  CFX_DateTime dt = CFX_DateTime::Now();
  EXPECT_EQ(2020, dt.GetYear());
  EXPECT_EQ(4, dt.GetMonth());
  EXPECT_EQ(23, dt.GetDay());
  EXPECT_EQ(15, dt.GetHour());
  EXPECT_EQ(5, dt.GetMinute());
  EXPECT_EQ(21, dt.GetSecond());
  EXPECT_EQ(0, dt.GetMillisecond());
}

TEST(CFXDateTimeTest, LeapYear) {
  EXPECT_TRUE(FX_IsLeapYear(2000));
  EXPECT_TRUE(FX_IsLeapYear(2024));
  EXPECT_FALSE(FX_IsLeapYear(1900));
  EXPECT_FALSE(FX_IsLeapYear(2023));
}

TEST(CFXDateTimeTest, DaysInMonth) {
  EXPECT_EQ(31, FX_DaysInMonth(2023, 1));
  EXPECT_EQ(28, FX_DaysInMonth(2023, 2));
  EXPECT_EQ(29, FX_DaysInMonth(2024, 2));
  EXPECT_EQ(31, FX_DaysInMonth(2023, 3));
  EXPECT_EQ(30, FX_DaysInMonth(2023, 4));
  EXPECT_EQ(31, FX_DaysInMonth(2023, 5));
  EXPECT_EQ(30, FX_DaysInMonth(2023, 6));
  EXPECT_EQ(31, FX_DaysInMonth(2023, 7));
  EXPECT_EQ(31, FX_DaysInMonth(2023, 8));
  EXPECT_EQ(30, FX_DaysInMonth(2023, 9));
  EXPECT_EQ(31, FX_DaysInMonth(2023, 10));
  EXPECT_EQ(30, FX_DaysInMonth(2023, 11));
  EXPECT_EQ(31, FX_DaysInMonth(2023, 12));
}

TEST(CFXDateTimeTest, DayOfWeek) {
  CFX_DateTime dt_now(2020, 4, 23, 0, 0, 0, 0);
  EXPECT_EQ(4, dt_now.GetDayOfWeek());

  // Year 1 AD.
  CFX_DateTime dt_ad1(1, 1, 1, 0, 0, 0, 0);
  EXPECT_EQ(1, dt_ad1.GetDayOfWeek());

  // Negative year (BCE).
  CFX_DateTime dt_bce(-1, 1, 1, 0, 0, 0, 0);
  EXPECT_EQ(0, dt_bce.GetDayOfWeek());

  // Negative leap year.
  CFX_DateTime dt_bce_leap(-4, 2, 29, 0, 0, 0, 0);
  EXPECT_EQ(6, dt_bce_leap.GetDayOfWeek());
}

TEST(CFXDateTimeTest, AccessorsAndReset) {
  CFX_DateTime dt;
  EXPECT_FALSE(dt.IsSet());
  EXPECT_EQ(0, dt.GetYear());

  dt.SetDate(2026, 11, 3);
  dt.SetTime(14, 30, 45, 123);
  EXPECT_TRUE(dt.IsSet());
  EXPECT_EQ(2026, dt.GetYear());
  EXPECT_EQ(11, dt.GetMonth());
  EXPECT_EQ(3, dt.GetDay());
  EXPECT_EQ(14, dt.GetHour());
  EXPECT_EQ(30, dt.GetMinute());
  EXPECT_EQ(45, dt.GetSecond());
  EXPECT_EQ(123, dt.GetMillisecond());

  CFX_DateTime dt_copy(2026, 11, 3, 14, 30, 45, 123);
  EXPECT_EQ(dt, dt_copy);

  dt.Reset();
  EXPECT_EQ(dt, CFX_DateTime());
  EXPECT_NE(dt, dt_copy);
}
