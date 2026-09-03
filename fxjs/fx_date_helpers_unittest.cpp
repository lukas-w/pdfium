// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/fx_date_helpers.h"

#include "core/fxcrt/fake_time_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr double kMilliSecondsInADay = 1000 * 60 * 60 * 24;

}  // namespace

using fxjs::ConversionStatus;

TEST(FXDateHelperTest, GetYearFromTime) {
  static constexpr struct {
    double time_ms;
    int expected_year;
  } kTests[] = {
      {-400 * kMilliSecondsInADay, 1968},
      {-1, 1969},
      {0, 1970},
      {1, 1970},
      {364.9 * kMilliSecondsInADay, 1970},
      {365.0 * kMilliSecondsInADay, 1971},
      {365.1 * kMilliSecondsInADay, 1971},
      {2 * 365.0 * kMilliSecondsInADay, 1972},
      // 1972 is a leap year, so there should be an extra day.
      {3 * 365.0 * kMilliSecondsInADay, 1972},
      {(3 * 365.0 + 1) * kMilliSecondsInADay, 1973},
      // 1900 is not a leap year.
      // There are 17 leap days between 1900 and 1970.
      {-(70 * 365.0 + 17) * kMilliSecondsInADay, 1900},
      {-(69 * 365.0 + 17 + 0.1) * kMilliSecondsInADay, 1900},
      {-(69 * 365.0 + 17) * kMilliSecondsInADay, 1901},
      // 2000 is a leap year.
      // There are 7 leap days between 1970 and 2000.
      {(30 * 365.0 + 7) * kMilliSecondsInADay, 2000},
      {(31 * 365.0 + 7) * kMilliSecondsInADay, 2000},
      {(31 * 365.0 + 8) * kMilliSecondsInADay, 2001},
  };

  for (const auto& test : kTests) {
    EXPECT_EQ(test.expected_year, FX_GetYearFromTime(test.time_ms))
        << test.time_ms;
  }
}

TEST(FXDateHelperTest, GetMonthFromTime) {
  static constexpr struct {
    double time_ms;
    int expected_month;  // Zero-based.
  } kTests[] = {
      {-400 * kMilliSecondsInADay, 10},
      {-1, 11},
      {0, 0},
      {1, 0},
      {364.9 * kMilliSecondsInADay, 11},
      {365.0 * kMilliSecondsInADay, 0},
      {365.1 * kMilliSecondsInADay, 0},
      // 1972 is a leap year, so there should be an extra day.
      {2 * 365.0 * kMilliSecondsInADay, 0},
      {3 * 365.0 * kMilliSecondsInADay, 11},
      {(3 * 365.0 + 1) * kMilliSecondsInADay, 0},
      // Tests boundaries for all months in 1970 not already covered above.
      {30 * kMilliSecondsInADay, 0},
      {31 * kMilliSecondsInADay, 1},
      {58 * kMilliSecondsInADay, 1},
      {59 * kMilliSecondsInADay, 2},
      {89 * kMilliSecondsInADay, 2},
      {90 * kMilliSecondsInADay, 3},
      {119 * kMilliSecondsInADay, 3},
      {120 * kMilliSecondsInADay, 4},
      {150 * kMilliSecondsInADay, 4},
      {151 * kMilliSecondsInADay, 5},
      {180 * kMilliSecondsInADay, 5},
      {181 * kMilliSecondsInADay, 6},
      {211 * kMilliSecondsInADay, 6},
      {212 * kMilliSecondsInADay, 7},
      {242 * kMilliSecondsInADay, 7},
      {243 * kMilliSecondsInADay, 8},
      {272 * kMilliSecondsInADay, 8},
      {273 * kMilliSecondsInADay, 9},
      {303 * kMilliSecondsInADay, 9},
      {304 * kMilliSecondsInADay, 10},
      {333 * kMilliSecondsInADay, 10},
      {334 * kMilliSecondsInADay, 11},
      {364 * kMilliSecondsInADay, 11},
      // Tests boundaries for all months in 1972 not already covered above.
      {(2 * 365.0 + 30) * kMilliSecondsInADay, 0},
      {(2 * 365.0 + 31) * kMilliSecondsInADay, 1},
      {(2 * 365.0 + 59) * kMilliSecondsInADay, 1},
      {(2 * 365.0 + 60) * kMilliSecondsInADay, 2},
      {(2 * 365.0 + 90) * kMilliSecondsInADay, 2},
      {(2 * 365.0 + 91) * kMilliSecondsInADay, 3},
      {(2 * 365.0 + 120) * kMilliSecondsInADay, 3},
      {(2 * 365.0 + 121) * kMilliSecondsInADay, 4},
      {(2 * 365.0 + 151) * kMilliSecondsInADay, 4},
      {(2 * 365.0 + 152) * kMilliSecondsInADay, 5},
      {(2 * 365.0 + 181) * kMilliSecondsInADay, 5},
      {(2 * 365.0 + 182) * kMilliSecondsInADay, 6},
      {(2 * 365.0 + 212) * kMilliSecondsInADay, 6},
      {(2 * 365.0 + 213) * kMilliSecondsInADay, 7},
      {(2 * 365.0 + 243) * kMilliSecondsInADay, 7},
      {(2 * 365.0 + 244) * kMilliSecondsInADay, 8},
      {(2 * 365.0 + 273) * kMilliSecondsInADay, 8},
      {(2 * 365.0 + 274) * kMilliSecondsInADay, 9},
      {(2 * 365.0 + 304) * kMilliSecondsInADay, 9},
      {(2 * 365.0 + 305) * kMilliSecondsInADay, 10},
      {(2 * 365.0 + 334) * kMilliSecondsInADay, 10},
      {(2 * 365.0 + 335) * kMilliSecondsInADay, 11},
      // 1900 is not a leap year.
      // There are 17 leap days between 1900 and 1970.
      {(-(70 * 365.0 + 17) + 58) * kMilliSecondsInADay, 1},
      {(-(70 * 365.0 + 17) + 59) * kMilliSecondsInADay, 2},
      // 2000 is a leap year.
      // There are 7 leap days between 1970 and 2000.
      {(30 * 365.0 + 7 + 58) * kMilliSecondsInADay, 1},
      {(30 * 365.0 + 7 + 59) * kMilliSecondsInADay, 1},
      {(30 * 365.0 + 7 + 60) * kMilliSecondsInADay, 2},
  };

  for (const auto& test : kTests) {
    EXPECT_EQ(test.expected_month, FX_GetMonthFromTime(test.time_ms))
        << test.time_ms;
  }
}

TEST(FXDateHelperTest, GetDayFromTime) {
  static constexpr struct {
    double time_ms;
    int expected_day;
  } kTests[] = {
      // 1900 is not a leap year.
      // There are 17 leap days between 1900 and 1970.
      {(-(70 * 365.0 + 17) + 58) * kMilliSecondsInADay, 28},
      {(-(70 * 365.0 + 17) + 59) * kMilliSecondsInADay, 1},
      // 1970 is not a leap year.
      {0, 1},
      {30 * kMilliSecondsInADay, 31},
      {31 * kMilliSecondsInADay, 1},
      // 1972 is a leap year.
      {(2 * 365.0 + 58) * kMilliSecondsInADay, 28},
      {(2 * 365.0 + 59) * kMilliSecondsInADay, 29},
      {(2 * 365.0 + 60) * kMilliSecondsInADay, 1},
      // 2000 is a leap year.
      // There are 7 leap days between 1970 and 2000.
      {(30 * 365.0 + 7 + 58) * kMilliSecondsInADay, 28},
      {(30 * 365.0 + 7 + 59) * kMilliSecondsInADay, 29},
      {(30 * 365.0 + 7 + 60) * kMilliSecondsInADay, 1},
  };

  for (const auto& test : kTests) {
    EXPECT_EQ(test.expected_day, FX_GetDayFromTime(test.time_ms))
        << test.time_ms;
  }
}

using FXDateHelperFakeTimeTest = FakeTimeTest;

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatWithEmptyParams) {
  double result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"", L"", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"value", L"", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"", L"format", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatForValidMonthDay) {
  double result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"01/02/2000", L"mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"1/2/2000", L"m/d/yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"1-2-2000", L"m-d-yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2-1-2000", L"d-m-yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);

  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"11/12/2000", L"mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"11/12/2000", L"m/d/yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"11-12-2000", L"m-d-yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"12-11-2000", L"d-m-yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);

  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"02/29/2000", L"mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(951'836'721'000, result);
}
