// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_CRT_CFGAS_DECIMAL_H_
#define XFA_FGAS_CRT_CFGAS_DECIMAL_H_

#include "core/fxcrt/widestring.h"

class CFGAS_Decimal {
 public:
  CFGAS_Decimal();
  explicit CFGAS_Decimal(uint32_t val);
  explicit CFGAS_Decimal(uint64_t val);
  explicit CFGAS_Decimal(int32_t val);
  CFGAS_Decimal(float val, uint8_t scale);
  explicit CFGAS_Decimal(WideStringView str);

  WideString ToWideString() const;
  float ToFloat() const;
  double ToDouble() const;

  CFGAS_Decimal operator*(const CFGAS_Decimal& val) const;
  CFGAS_Decimal operator/(const CFGAS_Decimal& val) const;

  bool IsNotZero() const { return hi_ || mid_ || lo_; }
  uint8_t GetScale() const { return u_scale_; }
  void SetScale(uint8_t newScale);
  void SetNegate();

 private:
  CFGAS_Decimal(uint32_t hi,
                uint32_t mid,
                uint32_t lo,
                bool neg,
                uint8_t scale);

  uint32_t hi_ = 0;
  uint32_t mid_ = 0;
  uint32_t lo_ = 0;
  bool neg_ = false;
  uint8_t u_scale_ = 0;
};

#endif  // XFA_FGAS_CRT_CFGAS_DECIMAL_H_
