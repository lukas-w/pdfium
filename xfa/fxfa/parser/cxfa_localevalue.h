// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_PARSER_CXFA_LOCALEVALUE_H_
#define XFA_FXFA_PARSER_CXFA_LOCALEVALUE_H_

#include <stdint.h>

#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/widestring.h"
#include "v8/include/cppgc/macros.h"
#include "xfa/fxfa/parser/cxfa_node.h"

class CFX_DateTime;
class CXFA_LocaleMgr;
class GCedLocaleIface;

class CXFA_LocaleValue {
  CPPGC_STACK_ALLOCATED();  // Raw/Unowned pointers allowed.

 public:
  enum class ValueType : uint8_t {
    kNull = 0,
    kBoolean,
    kInteger,
    kDecimal,
    kFloat,
    kText,
    kDate,
    kTime,
    kDateTime,
  };

  CXFA_LocaleValue();
  CXFA_LocaleValue(ValueType eType, CXFA_LocaleMgr* pLocaleMgr);
  CXFA_LocaleValue(ValueType eType,
                   const WideString& wsValue,
                   CXFA_LocaleMgr* pLocaleMgr);
  CXFA_LocaleValue(ValueType dwType,
                   const WideString& wsValue,
                   const WideString& wsFormat,
                   GCedLocaleIface* pLocale,
                   CXFA_LocaleMgr* pLocaleMgr);
  CXFA_LocaleValue(const CXFA_LocaleValue& that);
  ~CXFA_LocaleValue();

  CXFA_LocaleValue& operator=(const CXFA_LocaleValue& that);

  bool ValidateValue(const WideString& wsValue,
                     const WideString& wsPattern,
                     GCedLocaleIface* pLocale,
                     WideString* pMatchFormat);

  bool FormatPatterns(WideString& wsResult,
                      const WideString& wsFormat,
                      GCedLocaleIface* pLocale,
                      XFA_ValuePicture eValueType) const;

  void GetNumericFormat(WideString& wsFormat, int32_t nIntLen, int32_t nDecLen);
  bool ValidateNumericTemp(const WideString& wsNumeric,
                           const WideString& wsFormat,
                           GCedLocaleIface* pLocale);

  bool IsValid() const { return valid_; }
  const WideString& GetValue() const { return value_; }
  ValueType GetType() const { return type_; }
  double GetDoubleNum() const;
  void SetDate(const CFX_DateTime& d);
  CFX_DateTime GetDate() const;
  CFX_DateTime GetTime() const;

 private:
  bool FormatSinglePattern(WideString& wsResult,
                           const WideString& wsFormat,
                           GCedLocaleIface* pLocale,
                           XFA_ValuePicture eValueType) const;
  bool ValidateCanonicalValue(const WideString& wsValue, ValueType eType);
  bool ValidateCanonicalDate(const WideString& wsDate, CFX_DateTime* unDate);
  bool ValidateCanonicalTime(const WideString& wsTime);

  void SetTime(const CFX_DateTime& t);
  void SetDateTime(const CFX_DateTime& dt);

  bool ParsePatternValue(const WideString& wsValue,
                         const WideString& wsPattern,
                         GCedLocaleIface* pLocale);

  UnownedPtr<CXFA_LocaleMgr> locale_mgr_;  // Ok, stack-only.
  WideString value_;
  ValueType type_ = ValueType::kNull;
  bool valid_ = true;
};

#endif  // XFA_FXFA_PARSER_CXFA_LOCALEVALUE_H_
