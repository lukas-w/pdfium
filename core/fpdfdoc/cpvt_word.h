// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPVT_WORD_H_
#define CORE_FPDFDOC_CPVT_WORD_H_

#include <stdint.h>

#include "core/fpdfdoc/cpvt_wordplace.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_coordinates.h"

class CPVT_Word {
 public:
  CPVT_Word();
  CPVT_Word(const CPVT_Word&);
  CPVT_Word& operator=(const CPVT_Word&);
  ~CPVT_Word();

  uint16_t Word = 0;
  FX_Charset nCharset = FX_Charset::kANSI;
  CPVT_WordPlace WordPlace;
  CFX_PointF ptWord;
  float fAscent = 0.0f;
  float fDescent = 0.0f;
  float fWidth = 0.0f;
  int32_t nFontIndex = -1;
  float fFontSize = 0.0f;
};

#endif  // CORE_FPDFDOC_CPVT_WORD_H_
