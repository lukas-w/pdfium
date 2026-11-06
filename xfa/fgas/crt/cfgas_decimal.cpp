// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fgas/crt/cfgas_decimal.h"

#include <math.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_extension.h"

namespace {

constexpr int kDecimalScaleLimit = 0x1c;

constexpr uint32_t Rshift32Bit(uint64_t x) {
  return static_cast<uint32_t>(x >> 32);
}

constexpr uint64_t Lshift32Bit(uint64_t x) {
  return x << 32;
}

inline uint8_t decimal_helper_div10(uint64_t& phi,
                                    uint64_t& pmid,
                                    uint64_t& plo) {
  uint8_t retVal;
  pmid += Lshift32Bit(phi % 0xA);
  phi /= 0xA;
  plo += Lshift32Bit(pmid % 0xA);
  pmid /= 0xA;
  retVal = plo % 0xA;
  plo /= 0xA;
  return retVal;
}

inline uint8_t decimal_helper_div10_any(uint64_t nums[], uint8_t numcount) {
  uint8_t retVal = 0;
  UNSAFE_TODO({
    for (int i = numcount - 1; i > 0; i--) {
      nums[i - 1] += Lshift32Bit(nums[i] % 0xA);
      nums[i] /= 0xA;
    }
  });
  if (numcount) {
    retVal = nums[0] % 0xA;
    nums[0] /= 0xA;
  }
  return retVal;
}

inline void decimal_helper_mul10(uint64_t& phi, uint64_t& pmid, uint64_t& plo) {
  plo *= 0xA;
  pmid = pmid * 0xA + Rshift32Bit(plo);
  plo = static_cast<uint32_t>(plo);
  phi = phi * 0xA + Rshift32Bit(pmid);
  pmid = static_cast<uint32_t>(pmid);
}


inline void decimal_helper_normalize(uint64_t& phi,
                                     uint64_t& pmid,
                                     uint64_t& plo) {
  phi += Rshift32Bit(pmid);
  pmid = static_cast<uint32_t>(pmid);
  pmid += Rshift32Bit(plo);
  plo = static_cast<uint32_t>(plo);
  phi += Rshift32Bit(pmid);
  pmid = static_cast<uint32_t>(pmid);
}

inline void decimal_helper_normalize_any(uint64_t nums[], uint8_t len) {
  UNSAFE_TODO({
    for (int i = len - 2; i > 0; i--) {
      nums[i + 1] += Rshift32Bit(nums[i]);
      nums[i] = static_cast<uint32_t>(nums[i]);
    }
    for (int i = 0; i < len - 1; i++) {
      nums[i + 1] += Rshift32Bit(nums[i]);
      nums[i] = static_cast<uint32_t>(nums[i]);
    }
  });
}


inline void decimal_helper_inc_any(uint64_t a[], uint8_t al) {
  UNSAFE_TODO({
    for (int i = 0; i < al; i++) {
      a[i]++;
      if (static_cast<uint32_t>(a[i]) == a[i]) {
        return;
      }
      a[i] = 0;
    }
  });
}

inline void decimal_helper_raw_mul(uint64_t a[],
                                   uint8_t al,
                                   uint64_t b[],
                                   uint8_t bl,
                                   uint64_t c[],
                                   uint8_t cl) {
  DCHECK_LE(al + bl, cl);
  UNSAFE_TODO({
    for (int i = 0; i < cl; i++) {
      c[i] = 0;
    }

    for (int i = 0; i < al; i++) {
      for (int j = 0; j < bl; j++) {
        uint64_t m = static_cast<uint64_t>(a[i]) * b[j];
        c[i + j] += static_cast<uint32_t>(m);
        c[i + j + 1] += Rshift32Bit(m);
      }
    }
    for (int i = 0; i < cl - 1; i++) {
      c[i + 1] += Rshift32Bit(c[i]);
      c[i] = static_cast<uint32_t>(c[i]);
    }
    for (int i = 0; i < cl; i++) {
      c[i] = static_cast<uint32_t>(c[i]);
    }
  });
}

inline bool decimal_helper_outofrange(uint64_t a[], uint8_t al, uint8_t goal) {
  UNSAFE_TODO({
    for (int i = goal; i < al; i++) {
      if (a[i]) {
        return true;
      }
    }
  });
  return false;
}

inline void decimal_helper_shrinkintorange(uint64_t a[],
                                           uint8_t al,
                                           uint8_t goal,
                                           uint8_t& scale) {
  bool bRoundUp = false;
  while (scale != 0 && (scale > kDecimalScaleLimit ||
                        decimal_helper_outofrange(a, al, goal))) {
    bRoundUp = decimal_helper_div10_any(a, al) >= 5;
    scale--;
  }
  if (bRoundUp) {
    decimal_helper_normalize_any(a, goal);
    decimal_helper_inc_any(a, goal);
  }
}

}  // namespace

CFGAS_Decimal::CFGAS_Decimal() = default;

CFGAS_Decimal::CFGAS_Decimal(uint64_t val)
    : mid_(static_cast<uint32_t>(Rshift32Bit(val))),
      lo_(static_cast<uint32_t>(val)) {}

CFGAS_Decimal::CFGAS_Decimal(uint32_t val) : lo_(static_cast<uint32_t>(val)) {}

CFGAS_Decimal::CFGAS_Decimal(uint32_t lo,
                             uint32_t mid,
                             uint32_t hi,
                             bool neg,
                             uint8_t scale)
    : hi_(hi),
      mid_(mid),
      lo_(lo),
      neg_(neg && IsNotZero()),
      u_scale_(scale > kDecimalScaleLimit ? 0 : scale) {}

CFGAS_Decimal::CFGAS_Decimal(int32_t val) {
  if (val >= 0) {
    *this = CFGAS_Decimal(static_cast<uint32_t>(val));
  } else if (val == std::numeric_limits<int32_t>::min()) {
    *this = CFGAS_Decimal(static_cast<uint32_t>(val));
    SetNegate();
  } else {
    *this = CFGAS_Decimal(static_cast<uint32_t>(-val));
    SetNegate();
  }
}

CFGAS_Decimal::CFGAS_Decimal(float val, uint8_t scale) {
  float newval = fabs(val);
  float divisor = powf(2.0, 64.0f);
  uint64_t bottom64 = static_cast<uint64_t>(fmodf(newval, divisor));
  uint64_t top64 = static_cast<uint64_t>(newval / divisor);
  uint64_t plo = bottom64 & 0xFFFFFFFF;
  uint64_t pmid = bottom64 >> 32;
  uint64_t phi = top64 & 0xFFFFFFFF;

  newval = fmodf(newval, 1.0f);
  for (uint8_t iter = 0; iter < scale; iter++) {
    decimal_helper_mul10(phi, pmid, plo);
    newval *= 10;
    plo += static_cast<uint64_t>(newval);
    newval = fmodf(newval, 1.0f);
  }

  plo += FXSYS_roundf(newval);
  decimal_helper_normalize(phi, pmid, plo);
  hi_ = static_cast<uint32_t>(phi);
  mid_ = static_cast<uint32_t>(pmid);
  lo_ = static_cast<uint32_t>(plo);
  neg_ = val < 0 && IsNotZero();
  u_scale_ = scale;
}

CFGAS_Decimal::CFGAS_Decimal(WideStringView str) {
  bool pointmet = false;
  bool negmet = false;
  uint8_t scale = 0;
  // Note: Rely on WideStringView::Front() => NUL on empty strings.
  while (str.Front() == ' ') {
    str = str.Substr(1);
  }
  if (str.Front() == '-') {
    negmet = true;
    str = str.Substr(1);
  } else if (str.Front() == '+') {
    str = str.Substr(1);
  }
  while (scale < kDecimalScaleLimit) {
    if (str.Front() == '.') {
      pointmet = true;
      str = str.Substr(1);
      continue;
    }
    if (!FXSYS_IsDecimalDigit(static_cast<wchar_t>(str.Front()))) {
      break;
    }
    hi_ = hi_ * 0xA + Rshift32Bit(static_cast<uint64_t>(mid_) * 0xA);
    mid_ = mid_ * 0xA + Rshift32Bit(static_cast<uint64_t>(lo_) * 0xA);
    lo_ = lo_ * 0xA + (str.Front() - '0');
    if (pointmet) {
      scale++;
    }
    str = str.Substr(1);
  }
  neg_ = negmet && IsNotZero();
  u_scale_ = scale;
}

WideString CFGAS_Decimal::ToWideString() const {
  WideString retString;
  WideString tmpbuf;
  uint64_t phi = hi_;
  uint64_t pmid = mid_;
  uint64_t plo = lo_;
  while (phi || pmid || plo) {
    tmpbuf += decimal_helper_div10(phi, pmid, plo) + '0';
  }

  uint8_t outputlen = static_cast<uint8_t>(tmpbuf.GetLength());
  uint8_t scale = u_scale_;
  while (scale >= outputlen) {
    tmpbuf += '0';
    outputlen++;
  }
  if (neg_ && IsNotZero()) {
    retString += '-';
  }

  for (uint8_t idx = 0; idx < outputlen; idx++) {
    if (idx == (outputlen - scale) && scale != 0) {
      retString += '.';
    }
    retString += tmpbuf[outputlen - 1 - idx];
  }
  return retString;
}

float CFGAS_Decimal::ToFloat() const {
  return static_cast<float>(ToDouble());
}

double CFGAS_Decimal::ToDouble() const {
  constexpr double kPow32 = static_cast<double>(uint64_t{1} << 32);
  double base = static_cast<double>(hi_) * kPow32 * kPow32 +
                static_cast<double>(mid_) * kPow32 + static_cast<double>(lo_);
  return (neg_ ? -1 : 1) * base * pow(10.0, -u_scale_);
}

void CFGAS_Decimal::SetScale(uint8_t newscale) {
  uint8_t oldscale = u_scale_;
  if (oldscale == newscale) {
    return;
  }

  uint64_t phi = hi_;
  uint64_t pmid = mid_;
  uint64_t plo = lo_;
  if (newscale > oldscale) {
    for (uint8_t iter = 0; iter < newscale - oldscale; iter++) {
      decimal_helper_mul10(phi, pmid, plo);
    }

    hi_ = static_cast<uint32_t>(phi);
    mid_ = static_cast<uint32_t>(pmid);
    lo_ = static_cast<uint32_t>(plo);
    neg_ = neg_ && IsNotZero();
    u_scale_ = newscale;
  } else {
    uint64_t point5_hi = 0;
    uint64_t point5_mid = 0;
    uint64_t point5_lo = 5;
    for (uint8_t iter = 0; iter < oldscale - newscale - 1; iter++) {
      decimal_helper_mul10(point5_hi, point5_mid, point5_lo);
    }

    phi += point5_hi;
    pmid += point5_mid;
    plo += point5_lo;
    decimal_helper_normalize(phi, pmid, plo);
    for (uint8_t iter = 0; iter < oldscale - newscale; iter++) {
      decimal_helper_div10(phi, pmid, plo);
    }
  }
  hi_ = static_cast<uint32_t>(phi);
  mid_ = static_cast<uint32_t>(pmid);
  lo_ = static_cast<uint32_t>(plo);
  neg_ = neg_ && IsNotZero();
  u_scale_ = newscale;
}

void CFGAS_Decimal::SetNegate() {
  if (IsNotZero()) {
    neg_ = !neg_;
  }
}

CFGAS_Decimal CFGAS_Decimal::operator*(const CFGAS_Decimal& val) const {
  uint64_t a[3] = {lo_, mid_, hi_}, b[3] = {val.lo_, val.mid_, val.hi_};
  uint64_t c[6];
  decimal_helper_raw_mul(a, 3, b, 3, c, 6);
  bool neg = neg_ ^ val.neg_;
  uint8_t scale = u_scale_ + val.u_scale_;
  decimal_helper_shrinkintorange(c, 6, 3, scale);
  return CFGAS_Decimal(static_cast<uint32_t>(c[0]), static_cast<uint32_t>(c[1]),
                       static_cast<uint32_t>(c[2]), neg, scale);
}

void CFGAS_Decimal::DivBy10() {
  if (!IsNotZero()) {
    return;
  }

  uint64_t thi = hi_;
  uint64_t tmid = mid_;
  uint64_t tlo = lo_;
  uint8_t rem = decimal_helper_div10(thi, tmid, tlo);
  if (rem == 0) {
    hi_ = static_cast<uint32_t>(thi);
    mid_ = static_cast<uint32_t>(tmid);
    lo_ = static_cast<uint32_t>(tlo);
    return;
  }

  if (u_scale_ < kDecimalScaleLimit) {
    u_scale_++;
    return;
  }

  hi_ = static_cast<uint32_t>(thi);
  mid_ = static_cast<uint32_t>(tmid);
  lo_ = static_cast<uint32_t>(tlo);
  if (rem >= 5) {
    uint64_t plo = lo_ + 1;
    uint64_t pmid = mid_ + Rshift32Bit(plo);
    lo_ = static_cast<uint32_t>(plo);
    uint64_t phi = hi_ + Rshift32Bit(pmid);
    mid_ = static_cast<uint32_t>(pmid);
    hi_ = static_cast<uint32_t>(phi);
  }
}

void CFGAS_Decimal::DivBy100() {
  DivBy10();
  DivBy10();
}
