// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CPVT_WORD_H_
#define CORE_FPDFDOC_CPVT_WORD_H_

#include <stdint.h>

#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_coordinates.h"

class CPVT_Word {
 public:
  CPVT_Word();
  CPVT_Word(uint16_t word,
            FX_Charset charset,
            const CFX_PointF& location,
            float ascent,
            float descent,
            float width,
            int32_t font_index,
            float font_size);
  CPVT_Word(const CPVT_Word&);
  CPVT_Word& operator=(const CPVT_Word&);
  ~CPVT_Word();

  uint16_t word() const { return Word; }
  FX_Charset charset() const { return nCharset; }
  int32_t font_index() const { return nFontIndex; }
  const CFX_PointF& location() const { return ptWord; }
  void set_location(const CFX_PointF& location) { ptWord = location; }
  float width() const { return fWidth; }
  float font_size() const { return fFontSize; }
  float ascent() const { return fAscent; }
  float descent() const { return fDescent; }

  float AscentY() const { return ptWord.y + fAscent; }
  float DescentY() const { return ptWord.y + fDescent; }

 private:
  uint16_t Word = 0;
  FX_Charset nCharset = FX_Charset::kANSI;
  CFX_PointF ptWord;
  float fAscent = 0.0f;
  float fDescent = 0.0f;
  float fWidth = 0.0f;
  int32_t nFontIndex = -1;
  float fFontSize = 0.0f;
};

#endif  // CORE_FPDFDOC_CPVT_WORD_H_
