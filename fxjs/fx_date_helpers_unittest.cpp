// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxjs/fx_date_helpers.h"

#include <math.h>

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

TEST(FXDateHelperTest, GetDayOfWeekFromTime) {
  static constexpr struct {
    double time_ms;
    int expected_day_of_week;  // 0 = Sunday, 1 = Monday, ..., 6 = Saturday.
  } kTests[] = {
      {0, 4},                        // 1970-01-01 is Thursday.
      {1 * kMilliSecondsInADay, 5},  // 1970-01-02 is Friday.
      {2 * kMilliSecondsInADay, 6},  // 1970-01-03 is Saturday.
      {3 * kMilliSecondsInADay, 0},  // 1970-01-04 is Sunday.
      {4 * kMilliSecondsInADay, 1},  // 1970-01-05 is Monday.
      {5 * kMilliSecondsInADay, 2},  // 1970-01-06 is Tuesday.
      {6 * kMilliSecondsInADay, 3},  // 1970-01-07 is Wednesday.
      // Negative timestamps before 1970.
      {-1 * kMilliSecondsInADay, 3},  // 1969-12-31 is Wednesday.
      {-2 * kMilliSecondsInADay, 2},  // 1969-12-30 is Tuesday.
      {-3 * kMilliSecondsInADay, 1},  // 1969-12-29 is Monday.
      {-4 * kMilliSecondsInADay, 0},  // 1969-12-28 is Sunday.
      {-5 * kMilliSecondsInADay, 6},  // 1969-12-27 is Saturday.
  };

  for (const auto& test : kTests) {
    EXPECT_EQ(test.expected_day_of_week, FX_GetDayOfWeekFromTime(test.time_ms))
        << test.time_ms;
  }
}

TEST(FXDateHelperTest, GetHourFromTime) {
  EXPECT_EQ(0, FX_GetHourFromTime(0));
  EXPECT_EQ(1, FX_GetHourFromTime(3'600'000));
  EXPECT_EQ(12, FX_GetHourFromTime(12 * 3'600'000));
  EXPECT_EQ(23, FX_GetHourFromTime(23 * 3'600'000));
  EXPECT_EQ(0, FX_GetHourFromTime(24 * 3'600'000));
  EXPECT_EQ(23, FX_GetHourFromTime(-1));
  EXPECT_EQ(23, FX_GetHourFromTime(-3'600'000));
  EXPECT_EQ(22, FX_GetHourFromTime(-3'600'001));
}

TEST(FXDateHelperTest, GetMinFromTime) {
  EXPECT_EQ(0, FX_GetMinFromTime(0));
  EXPECT_EQ(1, FX_GetMinFromTime(60'000));
  EXPECT_EQ(30, FX_GetMinFromTime(30 * 60'000));
  EXPECT_EQ(59, FX_GetMinFromTime(59 * 60'000));
  EXPECT_EQ(0, FX_GetMinFromTime(60 * 60'000));
  EXPECT_EQ(59, FX_GetMinFromTime(-1));
  EXPECT_EQ(59, FX_GetMinFromTime(-60'000));
  EXPECT_EQ(58, FX_GetMinFromTime(-60'001));
}

TEST(FXDateHelperTest, GetSecFromTime) {
  EXPECT_EQ(0, FX_GetSecFromTime(0));
  EXPECT_EQ(1, FX_GetSecFromTime(1'000));
  EXPECT_EQ(45, FX_GetSecFromTime(45 * 1'000));
  EXPECT_EQ(59, FX_GetSecFromTime(59 * 1'000));
  EXPECT_EQ(0, FX_GetSecFromTime(60 * 1'000));
  EXPECT_EQ(59, FX_GetSecFromTime(-1));
  EXPECT_EQ(59, FX_GetSecFromTime(-1'000));
  EXPECT_EQ(58, FX_GetSecFromTime(-1'001));
}

TEST(FXDateHelperTest, ValidationHelpers) {
  EXPECT_FALSE(FX_IsValidMonth(0));
  EXPECT_TRUE(FX_IsValidMonth(1));
  EXPECT_TRUE(FX_IsValidMonth(12));
  EXPECT_FALSE(FX_IsValidMonth(13));
  EXPECT_FALSE(FX_IsValidMonth(-1));

  EXPECT_FALSE(FX_IsValidDay(0));
  EXPECT_TRUE(FX_IsValidDay(1));
  EXPECT_TRUE(FX_IsValidDay(31));
  EXPECT_FALSE(FX_IsValidDay(32));
  EXPECT_FALSE(FX_IsValidDay(-1));

  EXPECT_FALSE(FX_IsValid24Hour(-1));
  EXPECT_TRUE(FX_IsValid24Hour(0));
  EXPECT_TRUE(FX_IsValid24Hour(24));
  EXPECT_FALSE(FX_IsValid24Hour(25));

  EXPECT_FALSE(FX_IsValidMinute(-1));
  EXPECT_TRUE(FX_IsValidMinute(0));
  EXPECT_TRUE(FX_IsValidMinute(60));
  EXPECT_FALSE(FX_IsValidMinute(61));

  EXPECT_FALSE(FX_IsValidSecond(-1));
  EXPECT_TRUE(FX_IsValidSecond(0));
  EXPECT_TRUE(FX_IsValidSecond(60));
  EXPECT_FALSE(FX_IsValidSecond(61));
}

TEST(FXDateHelperTest, MakeDay) {
  EXPECT_DOUBLE_EQ(0.0, FX_MakeDay(1970, 0, 1));
  EXPECT_DOUBLE_EQ(1.0, FX_MakeDay(1970, 0, 2));
  EXPECT_DOUBLE_EQ(31.0, FX_MakeDay(1970, 1, 1));
  EXPECT_DOUBLE_EQ(2 * 365 + 31 + 28, FX_MakeDay(1972, 1, 29));
  EXPECT_DOUBLE_EQ(2 * 365 + 31 + 29, FX_MakeDay(1972, 2, 1));
  EXPECT_DOUBLE_EQ(365.0, FX_MakeDay(1970, 12, 1));
  EXPECT_DOUBLE_EQ(-31.0, FX_MakeDay(1970, -1, 1));
}

TEST(FXDateHelperTest, MakeTime) {
  EXPECT_DOUBLE_EQ(0.0, FX_MakeTime(0, 0, 0, 0));
  EXPECT_DOUBLE_EQ(3'723'004.0, FX_MakeTime(1, 2, 3, 4));
}

TEST(FXDateHelperTest, MakeDate) {
  EXPECT_DOUBLE_EQ(0.0, FX_MakeDate(0, 0));
  EXPECT_DOUBLE_EQ(86'400'500.0, FX_MakeDate(1, 500));
  EXPECT_TRUE(isnan(FX_MakeDate(nan(""), 0)));
  EXPECT_TRUE(isnan(FX_MakeDate(0, nan(""))));
  EXPECT_TRUE(isnan(FX_MakeDate(INFINITY, 0)));
  EXPECT_TRUE(isnan(FX_MakeDate(0, INFINITY)));
}

TEST(FXDateHelperTest, ParseStringInteger) {
  size_t skip = 0;
  EXPECT_EQ(123, FX_ParseStringInteger(L"12345", 0, &skip, 3));
  EXPECT_EQ(3u, skip);

  skip = 0;
  EXPECT_EQ(12, FX_ParseStringInteger(L"12345", 0, &skip, 2));
  EXPECT_EQ(2u, skip);

  skip = 0;
  EXPECT_EQ(12, FX_ParseStringInteger(L"12abc", 0, &skip, 4));
  EXPECT_EQ(2u, skip);

  skip = 1;
  EXPECT_EQ(0, FX_ParseStringInteger(L"abc", 0, &skip, 2));
  EXPECT_EQ(0u, skip);

  skip = 1;
  EXPECT_EQ(0, FX_ParseStringInteger(L"123", 5, &skip, 2));
  EXPECT_EQ(0u, skip);

  skip = 0;
  EXPECT_EQ(123456789, FX_ParseStringInteger(L"123456789abc", 0, &skip, 15));
  EXPECT_EQ(9u, skip);
}

using FXDateHelperFakeTimeTest = FakeTimeTest;

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatWithEmptyParams) {
  double result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"", L"", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"value", L"", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"", L"format", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatForValidMonthDay) {
  double result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"01/02/2000", L"mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"1/2/2000", L"m/d/yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"1-2-2000", L"m-d-yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2-1-2000", L"d-m-yyyy", &result));
  EXPECT_DOUBLE_EQ(946'825'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"11/12/2000", L"mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"11/12/2000", L"m/d/yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"11-12-2000", L"m-d-yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"12-11-2000", L"d-m-yyyy", &result));
  EXPECT_DOUBLE_EQ(974'041'521'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"02/29/2000", L"mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(951'836'721'000, result);
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatFor12HourTime) {
  double result = 0.0;

  // 12:30:00 PM is 12:30.
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 pm",
                                    L"yyyy/mm/dd hh:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'645'000'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 PM",
                                    L"yyyy/mm/dd hh:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'645'000'000, result);

  // 12:30:00 AM is 00:30.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 am",
                                    L"yyyy/mm/dd hh:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'601'800'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 AM",
                                    L"yyyy/mm/dd hh:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'601'800'000, result);

  // 1:30:00 PM is 13:30.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 pm",
                                    L"yyyy/mm/dd h:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'648'600'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 PM",
                                    L"yyyy/mm/dd h:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'648'600'000, result);

  // 1:30:00 AM is 01:30.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 am",
                                    L"yyyy/mm/dd h:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'605'400'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 AM",
                                    L"yyyy/mm/dd h:MM:ss tt", &result));
  EXPECT_DOUBLE_EQ(1'587'605'400'000, result);

  // Single 't' marker.
  // 12:30:00 P is 12:30.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 p",
                                    L"yyyy/mm/dd hh:MM:ss t", &result));
  EXPECT_DOUBLE_EQ(1'587'645'000'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 P",
                                    L"yyyy/mm/dd hh:MM:ss t", &result));
  EXPECT_DOUBLE_EQ(1'587'645'000'000, result);

  // 12:30:00 A is 00:30.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 a",
                                    L"yyyy/mm/dd hh:MM:ss t", &result));
  EXPECT_DOUBLE_EQ(1'587'601'800'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020/04/23 12:30:00 A",
                                    L"yyyy/mm/dd hh:MM:ss t", &result));
  EXPECT_DOUBLE_EQ(1'587'601'800'000, result);

  // Malformed AM/PM markers should return kBadFormat.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadFormat,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 xm",
                                    L"yyyy/mm/dd h:MM:ss tt", &result));
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadFormat,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 px",
                                    L"yyyy/mm/dd h:MM:ss tt", &result));
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadFormat,
            FX_ParseDateUsingFormat(L"2020/04/23 1:30:00 x",
                                    L"yyyy/mm/dd h:MM:ss t", &result));
}

TEST_F(FXDateHelperFakeTimeTest, GetDateTime) {
  EXPECT_DOUBLE_EQ(1'587'654'321'000.0, FX_GetDateTime());
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatForFullMonthNames) {
  double result = 0.0;
  static constexpr struct {
    const wchar_t* input;
    double expected;
  } kTests[] = {
      {L"January 15, 2020", 1'579'100'721'000},
      {L"February 15, 2020", 1'581'779'121'000},
      {L"March 15, 2020", 1'584'284'721'000},
      {L"April 15, 2020", 1'586'963'121'000},
      {L"May 15, 2020", 1'589'555'121'000},
      {L"June 15, 2020", 1'592'233'521'000},
      {L"July 15, 2020", 1'594'825'521'000},
      {L"August 15, 2020", 1'597'503'921'000},
      {L"September 15, 2020", 1'600'182'321'000},
      {L"October 15, 2020", 1'602'774'321'000},
      {L"November 15, 2020", 1'605'452'721'000},
      {L"December 15, 2020", 1'608'044'721'000},
  };

  for (const auto& test : kTests) {
    result = 0.0;
    EXPECT_EQ(ConversionStatus::kSuccess,
              FX_ParseDateUsingFormat(test.input, L"mmmm dd, yyyy", &result))
        << test.input;
    EXPECT_DOUBLE_EQ(test.expected, result) << test.input;
  }
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatForAbbreviatedMonthNames) {
  double result = 0.0;
  static constexpr struct {
    const wchar_t* input;
    double expected;
  } kTests[] = {
      {L"Jan 15, 2020", 1'579'100'721'000},
      {L"feb 15, 2020", 1'581'779'121'000},
      {L"MAR 15, 2020", 1'584'284'721'000},
      {L"Apr 15, 2020", 1'586'963'121'000},
      {L"May 15, 2020", 1'589'555'121'000},
      {L"Jun 15, 2020", 1'592'233'521'000},
      {L"Jul 15, 2020", 1'594'825'521'000},
      {L"Aug 15, 2020", 1'597'503'921'000},
      {L"Sep 15, 2020", 1'600'182'321'000},
      {L"Oct 15, 2020", 1'602'774'321'000},
      {L"Nov 15, 2020", 1'605'452'721'000},
      {L"Dec 15, 2020", 1'608'044'721'000},
  };

  for (const auto& test : kTests) {
    result = 0.0;
    EXPECT_EQ(ConversionStatus::kSuccess,
              FX_ParseDateUsingFormat(test.input, L"mmm dd, yyyy", &result))
        << test.input;
    EXPECT_DOUBLE_EQ(test.expected, result) << test.input;
  }
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatNumericMonthFallback) {
  double result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"03 15, 2020", L"mmm dd, yyyy", &result));
  EXPECT_DOUBLE_EQ(1'584'284'721'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"07 15, 2020", L"mmmm dd, yyyy", &result));
  EXPECT_DOUBLE_EQ(1'594'825'521'000, result);
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatTwoDigitYear) {
  double result = 0.0;
  // Year < 50 maps to 2000s.
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"04/23/20", L"mm/dd/yy", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"04/23/49", L"mm/dd/yy", &result));
  EXPECT_DOUBLE_EQ(2'502'803'121'000, result);

  // Year >= 50 maps to 1900s.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"04/23/50", L"mm/dd/yy", &result));
  EXPECT_DOUBLE_EQ(-621'420'879'000, result);

  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"04/23/99", L"mm/dd/yy", &result));
  EXPECT_DOUBLE_EQ(924'879'921'000, result);

  // 1-digit year 'y' skips 1 character, retaining current year.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"x 04/23", L"y mm/dd", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatSeparatorsAndLiterals) {
  double result = 0.0;
  // Dots, colons, and 24-hour 'HH'.
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020.04.23 18:30:45",
                                    L"yyyy.mm.dd HH:MM:ss", &result));
  EXPECT_DOUBLE_EQ(1'587'666'645'000, result);

  // Backslashes.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020\\04\\23", L"yyyy\\mm\\dd", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  // token_len == 3 with non-m/y format character skips 3 characters.
  result = 0.0;
  EXPECT_EQ(
      ConversionStatus::kSuccess,
      FX_ParseDateUsingFormat(L"abc/04/23/2020", L"ddd/mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  // token_len == 4 with non-m/y format character skips 4 characters.
  result = 0.0;
  EXPECT_EQ(
      ConversionStatus::kSuccess,
      FX_ParseDateUsingFormat(L"abcd/04/23/2020", L"dddd/mm/dd/yyyy", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  // token_len > 4 matching literal characters.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"ddddd/04/23/2020", L"ddddd/mm/dd/yyyy",
                                    &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  // Separators are interchangeable: '-' in value matches '/' in format.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kSuccess,
            FX_ParseDateUsingFormat(L"2020-04-23", L"yyyy/mm/dd", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);

  // Trailing format characters when value ends are ignored.
  result = 0.0;
  EXPECT_EQ(
      ConversionStatus::kSuccess,
      FX_ParseDateUsingFormat(L"2020/04/23", L"yyyy/mm/dd Extra", &result));
  EXPECT_DOUBLE_EQ(1'587'654'321'000, result);
}

TEST_F(FXDateHelperFakeTimeTest, ParseDateUsingFormatErrors) {
  double result = 0.0;
  // Literal mismatch.
  EXPECT_EQ(ConversionStatus::kBadFormat,
            FX_ParseDateUsingFormat(L"2020/04/23 X 12:00:00",
                                    L"yyyy/mm/dd T HH:MM:ss", &result));

  // token_len > 4 literal mismatch.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadFormat,
            FX_ParseDateUsingFormat(L"12345/04/23/2020", L"ddddd/mm/dd/yyyy",
                                    &result));

  // 'yyy' token in format does not advance value_idx, triggering bad format.
  result = 0.0;
  EXPECT_EQ(
      ConversionStatus::kBadFormat,
      FX_ParseDateUsingFormat(L"abc 04/23/2020", L"yyy mm/dd/yyyy", &result));

  // Long non-month word exceeding kLongestFullMonthLength.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadFormat,
            FX_ParseDateUsingFormat(L"Superlonginvalidmonth 15, 2020",
                                    L"mmmm dd, yyyy", &result));

  // Invalid date components (kBadDate):
  // Month out of range (13).
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/13/15", L"yyyy/mm/dd", &result));

  // Month 0.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/00/15", L"yyyy/mm/dd", &result));

  // Day out of range (32).
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/04/32", L"yyyy/mm/dd", &result));

  // Day 0.
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/04/00", L"yyyy/mm/dd", &result));

  // Hour out of range (25).
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/04/23 25:00:00",
                                    L"yyyy/mm/dd HH:MM:ss", &result));

  // Minute out of range (61).
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/04/23 12:61:00",
                                    L"yyyy/mm/dd HH:MM:ss", &result));

  // Second out of range (61).
  result = 0.0;
  EXPECT_EQ(ConversionStatus::kBadDate,
            FX_ParseDateUsingFormat(L"2020/04/23 12:00:61",
                                    L"yyyy/mm/dd HH:MM:ss", &result));
}
