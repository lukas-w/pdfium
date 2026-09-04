// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/fx_date_helpers.h"

#include <math.h>
#include <time.h>
#include <wctype.h>

#include <array>
#include <iterator>

#include "build/build_config.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_system.h"
#include "fpdfsdk/cpdfsdk_helpers.h"

namespace fxjs {
namespace {

constexpr std::array<uint16_t, 12> kDaysMonth = {
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334}};

constexpr std::array<uint16_t, 12> kLeapDaysMonth = {
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};

double Mod(double x, double y) {
  double r = fmod(x, y);
  if (r < 0) {
    r += y;
  }
  return r;
}

double GetLocalTZA() {
  if (!IsPDFSandboxPolicyEnabled(FPDF_POLICY_MACHINETIME_ACCESS)) {
    return 0;
  }
  time_t t = 0;
  FXSYS_time(&t);
  FXSYS_localtime(&t);
#if BUILDFLAG(IS_WIN)
  // In gcc 'timezone' is a global variable declared in time.h. In VC++, that
  // variable was removed in VC++ 2015, with _get_timezone replacing it.
  long timezone = 0;
  _get_timezone(&timezone);
#endif
  return (double)(-(timezone * 1000));
}

int GetDaylightSavingTA(double d) {
  if (!IsPDFSandboxPolicyEnabled(FPDF_POLICY_MACHINETIME_ACCESS)) {
    return 0;
  }
  time_t t = (time_t)(d / 1000);
  struct tm* tmp = FXSYS_localtime(&t);
  if (!tmp) {
    return 0;
  }
  if (tmp->tm_isdst > 0) {
    // One hour.
    return (int)60 * 60 * 1000;
  }
  return 0;
}

bool IsLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DayFromYear(int y) {
  return (int)(365 * (y - 1970.0) + floor((y - 1969.0) / 4) -
               floor((y - 1901.0) / 100) + floor((y - 1601.0) / 400));
}

double TimeFromYear(int y) {
  return 86400000.0 * DayFromYear(y);
}

double TimeFromYearMonth(int y, int m) {
  const uint16_t month = IsLeapYear(y) ? kLeapDaysMonth[m] : kDaysMonth[m];
  return TimeFromYear(y) + static_cast<double>(month) * 86400000;
}

int Day(double t) {
  return static_cast<int>(floor(t / 86400000.0));
}

int YearFromTime(double t) {
  // estimate the time.
  int y = 1970 + static_cast<int>(t / (365.2425 * 86400000.0));
  if (TimeFromYear(y) <= t) {
    while (TimeFromYear(y + 1) <= t) {
      y++;
    }
  } else {
    while (TimeFromYear(y) > t) {
      y--;
    }
  }
  return y;
}

int DayWithinYear(double t) {
  int year = YearFromTime(t);
  int day = Day(t);
  return day - DayFromYear(year);
}

int MonthFromTime(double t) {
  // Check for negative |day| values and check for January.
  int day = DayWithinYear(t);
  if (day < 0) {
    return -1;
  }
  if (day < 31) {
    return 0;
  }

  if (IsLeapYear(YearFromTime(t))) {
    --day;
  }

  // Check for February onwards.
  static constexpr std::array<int, 11> kCumulativeDaysInMonths = {
      {59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365}};
  for (size_t i = 0; i < std::size(kCumulativeDaysInMonths); ++i) {
    if (day < kCumulativeDaysInMonths[i]) {
      return static_cast<int>(i) + 1;
    }
  }

  return -1;
}

int DateFromTime(double t) {
  int day = DayWithinYear(t);
  int year = YearFromTime(t);
  int leap = IsLeapYear(year);
  int month = MonthFromTime(t);
  switch (month) {
    case 0:
      return day + 1;
    case 1:
      return day - 30;
    case 2:
      return day - 58 - leap;
    case 3:
      return day - 89 - leap;
    case 4:
      return day - 119 - leap;
    case 5:
      return day - 150 - leap;
    case 6:
      return day - 180 - leap;
    case 7:
      return day - 211 - leap;
    case 8:
      return day - 242 - leap;
    case 9:
      return day - 272 - leap;
    case 10:
      return day - 303 - leap;
    case 11:
      return day - 333 - leap;
    default:
      return 0;
  }
}

size_t FindSubWordLength(const WideString& str, size_t nStart) {
  pdfium::span<const wchar_t> data = str.span();
  size_t i = nStart;
  while (i < data.size() && iswalnum(data[i])) {
    ++i;
  }
  return i - nStart;
}

}  // namespace

const std::array<const char*, 12> kMonths = {{"Jan", "Feb", "Mar", "Apr", "May",
                                              "Jun", "Jul", "Aug", "Sep", "Oct",
                                              "Nov", "Dec"}};

const std::array<const char*, 12> kFullMonths = {
    {"January", "February", "March", "April", "May", "June", "July", "August",
     "September", "October", "November", "December"}};

static constexpr size_t KMonthAbbreviationLength = 3;  // Anything in |kMonths|.
static constexpr size_t kLongestFullMonthLength = 9;   // September

double FX_GetDateTime() {
  if (!IsPDFSandboxPolicyEnabled(FPDF_POLICY_MACHINETIME_ACCESS)) {
    return 0;
  }

  time_t t = FXSYS_time(nullptr);
  struct tm* pTm = FXSYS_localtime(&t);
  double t1 = TimeFromYear(pTm->tm_year + 1900);
  return t1 + pTm->tm_yday * 86400000.0 + pTm->tm_hour * 3600000.0 +
         pTm->tm_min * 60000.0 + pTm->tm_sec * 1000.0;
}

int FX_GetYearFromTime(double dt) {
  return YearFromTime(dt);
}

int FX_GetMonthFromTime(double dt) {
  return MonthFromTime(dt);
}

int FX_GetDayFromTime(double dt) {
  return DateFromTime(dt);
}

int FX_GetDayOfWeekFromTime(double dt) {
  // 1970-01-01 was Thursday (day 4 in 0-based Sunday-indexed week).
  return static_cast<int>(Mod(Day(dt) + 4, 7));
}

int FX_GetHourFromTime(double dt) {
  return (int)Mod(floor(dt / (60 * 60 * 1000)), 24);
}

int FX_GetMinFromTime(double dt) {
  return (int)Mod(floor(dt / (60 * 1000)), 60);
}

int FX_GetSecFromTime(double dt) {
  return (int)Mod(floor(dt / 1000), 60);
}

bool FX_IsValidMonth(int m) {
  return m >= 1 && m <= 12;
}

// TODO(thestig): Should this take the month into consideration?
bool FX_IsValidDay(int d) {
  return d >= 1 && d <= 31;
}

// TODO(thestig): Should 24 be allowed? Similarly, 60 for minutes and seconds.
bool FX_IsValid24Hour(int h) {
  return h >= 0 && h <= 24;
}

bool FX_IsValidMinute(int m) {
  return m >= 0 && m <= 60;
}

bool FX_IsValidSecond(int s) {
  return s >= 0 && s <= 60;
}

double FX_LocalTime(double d) {
  return d + GetLocalTZA() + GetDaylightSavingTA(d);
}

double FX_MakeDay(int nYear, int nMonth, int nDate) {
  double y = static_cast<double>(nYear);
  double m = static_cast<double>(nMonth);
  double dt = static_cast<double>(nDate);
  double ym = y + floor(m / 12);
  double mn = Mod(m, 12);
  double t = TimeFromYearMonth(static_cast<int>(ym), static_cast<int>(mn));
  if (YearFromTime(t) != ym || MonthFromTime(t) != mn || DateFromTime(t) != 1) {
    return nan("");
  }

  return Day(t) + dt - 1;
}

double FX_MakeTime(int nHour, int nMin, int nSec, int nMs) {
  double h = static_cast<double>(nHour);
  double m = static_cast<double>(nMin);
  double s = static_cast<double>(nSec);
  double milli = static_cast<double>(nMs);
  return h * 3600000 + m * 60000 + s * 1000 + milli;
}

double FX_MakeDate(double day, double time) {
  if (!isfinite(day) || !isfinite(time)) {
    return nan("");
  }

  return day * 86400000 + time;
}

int FX_ParseStringInteger(const WideString& str,
                          size_t nStart,
                          size_t* pSkip,
                          size_t nMaxStep) {
  int nRet = 0;
  size_t nSkip = 0;
  for (size_t i = nStart; i < str.GetLength(); ++i) {
    if (i - nStart > 10) {
      break;
    }

    wchar_t c = str[i];
    if (!FXSYS_IsDecimalDigit(c)) {
      break;
    }

    nRet = nRet * 10 + FXSYS_DecimalCharToInt(c);
    ++nSkip;
    if (nSkip >= nMaxStep) {
      break;
    }
  }

  *pSkip = nSkip;
  return nRet;
}

ConversionStatus FX_ParseDateUsingFormat(const WideString& value,
                                         const WideString& format,
                                         double* result) {
  const double dt = FX_GetDateTime();
  if (format.IsEmpty() || value.IsEmpty()) {
    *result = dt;
    return ConversionStatus::kSuccess;
  }

  int year = FX_GetYearFromTime(dt);
  int month = FX_GetMonthFromTime(dt) + 1;
  int day = FX_GetDayFromTime(dt);
  int hour = FX_GetHourFromTime(dt);
  int minute = FX_GetMinFromTime(dt);
  int second = FX_GetSecFromTime(dt);
  bool is_pm = false;
  bool exit_loop = false;
  bool bad_format = false;
  size_t format_idx = 0;
  size_t value_idx = 0;

  while (format_idx < format.GetLength()) {
    if (exit_loop) {
      break;
    }

    const wchar_t format_char = format[format_idx];
    switch (format_char) {
      case ':':
      case '.':
      case '-':
      case '\\':
      case '/':
        ++format_idx;
        ++value_idx;
        break;

      case 'y':
      case 'm':
      case 'd':
      case 'H':
      case 'h':
      case 'M':
      case 's':
      case 't': {
        const size_t old_value_idx = value_idx;
        size_t chars_to_skip = 0;
        const size_t remaining = format.GetLength() - format_idx - 1;

        if (remaining == 0 || format[format_idx + 1] != format_char) {
          switch (format_char) {
            case 'y':
              ++format_idx;
              ++value_idx;
              break;
            case 'm':
              month =
                  FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              ++format_idx;
              value_idx += chars_to_skip;
              break;
            case 'd':
              day = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              ++format_idx;
              value_idx += chars_to_skip;
              break;
            case 'H':
              hour = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              ++format_idx;
              value_idx += chars_to_skip;
              break;
            case 'h':
              hour = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              ++format_idx;
              value_idx += chars_to_skip;
              break;
            case 'M':
              minute =
                  FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              ++format_idx;
              value_idx += chars_to_skip;
              break;
            case 's':
              second =
                  FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              ++format_idx;
              value_idx += chars_to_skip;
              break;
            case 't':
              is_pm =
                  (value_idx < value.GetLength() && value[value_idx] == 'p');
              ++format_idx;
              ++value_idx;
              break;
          }
        } else if (remaining == 1 || format[format_idx + 2] != format_char) {
          switch (format_char) {
            case 'y':
              year = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 'm':
              month =
                  FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 'd':
              day = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 'H':
              hour = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 'h':
              hour = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 'M':
              minute =
                  FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 's':
              second =
                  FX_ParseStringInteger(value, value_idx, &chars_to_skip, 2);
              format_idx += 2;
              value_idx += chars_to_skip;
              break;
            case 't':
              is_pm = (value_idx + 1 < value.GetLength() &&
                       value[value_idx] == 'p' && value[value_idx + 1] == 'm');
              format_idx += 2;
              value_idx += 2;
              break;
          }
        } else if (remaining == 2 || format[format_idx + 3] != format_char) {
          switch (format_char) {
            case 'm': {
              bool found = false;
              chars_to_skip = FindSubWordLength(value, value_idx);
              if (chars_to_skip == KMonthAbbreviationLength) {
                const WideString month_str =
                    value.Substr(value_idx, KMonthAbbreviationLength);
                for (size_t m = 0; m < std::size(kMonths); ++m) {
                  if (month_str.EqualsASCIINoCase(kMonths[m])) {
                    month = static_cast<int>(m) + 1;
                    format_idx += 3;
                    value_idx += chars_to_skip;
                    found = true;
                    break;
                  }
                }
              }

              if (!found) {
                month =
                    FX_ParseStringInteger(value, value_idx, &chars_to_skip, 3);
                format_idx += 3;
                value_idx += chars_to_skip;
              }
              break;
            }
            case 'y':
              break;
            default:
              format_idx += 3;
              value_idx += 3;
              break;
          }
        } else if (remaining == 3 || format[format_idx + 4] != format_char) {
          switch (format_char) {
            case 'y':
              year = FX_ParseStringInteger(value, value_idx, &chars_to_skip, 4);
              format_idx += 4;
              value_idx += chars_to_skip;
              break;
            case 'm': {
              bool found = false;
              chars_to_skip = FindSubWordLength(value, value_idx);
              if (chars_to_skip <= kLongestFullMonthLength) {
                WideString month_str = value.Substr(value_idx, chars_to_skip);
                month_str.MakeLower();
                for (size_t m = 0; m < std::size(kFullMonths); ++m) {
                  auto full_month = WideString::FromASCII(kFullMonths[m]);
                  full_month.MakeLower();
                  if (full_month.Contains(month_str.AsStringView())) {
                    month = static_cast<int>(m) + 1;
                    format_idx += 4;
                    value_idx += chars_to_skip;
                    found = true;
                    break;
                  }
                }
              }
              if (!found) {
                month =
                    FX_ParseStringInteger(value, value_idx, &chars_to_skip, 4);
                format_idx += 4;
                value_idx += chars_to_skip;
              }
              break;
            }
            default:
              format_idx += 4;
              value_idx += 4;
              break;
          }
        } else {
          if (value_idx >= value.GetLength() ||
              format[format_idx] != value[value_idx]) {
            bad_format = true;
            exit_loop = true;
          }
          ++format_idx;
          ++value_idx;
        }

        if (old_value_idx == value_idx) {
          bad_format = true;
          exit_loop = true;
        }
        break;
      }

      default:
        if (value.GetLength() <= value_idx) {
          exit_loop = true;
        } else if (format[format_idx] != value[value_idx]) {
          bad_format = true;
          exit_loop = true;
        }

        ++format_idx;
        ++value_idx;
        break;
    }
  }

  if (bad_format) {
    return ConversionStatus::kBadFormat;
  }

  if (is_pm) {
    hour += 12;
  }

  // Resolves two-digit year ambiguity using Acrobat's date horizon heuristic:
  // < 50 is assumed in the 21st century (+2000), >= 50 in the 20th century
  // (+1900).
  if (year >= 0 && year < 100) {
    year += year < 50 ? 2000 : 1900;
  }

  if (!FX_IsValidMonth(month) || !FX_IsValidDay(day) ||
      !FX_IsValid24Hour(hour) || !FX_IsValidMinute(minute) ||
      !FX_IsValidSecond(second)) {
    return ConversionStatus::kBadDate;
  }

  const double parsed_dt = FX_MakeDate(FX_MakeDay(year, month - 1, day),
                                       FX_MakeTime(hour, minute, second, 0));
  if (isnan(parsed_dt)) {
    return ConversionStatus::kBadDate;
  }

  *result = parsed_dt;
  return ConversionStatus::kSuccess;
}

}  // namespace fxjs
