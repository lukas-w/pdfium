// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FGAS_FONT_CFGAS_PDFFONTMGR_H_
#define XFA_FGAS_FONT_CFGAS_PDFFONTMGR_H_

#include <map>
#include <utility>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/retain_ptr.h"
#include "xfa/fgas/font/cfgas_fontmgr.h"

class CFGAS_GEFont;
class CPDF_Document;

class CFGAS_PDFFontMgr final {
 public:
  CFGAS_PDFFontMgr();
  ~CFGAS_PDFFontMgr();

  RetainPtr<CFGAS_GEFont> GetFont(const CPDF_Document* doc,
                                  const WideString& wsFontFamily,
                                  uint32_t dwFontStyles,
                                  bool bStrictMatch);

 private:
  RetainPtr<CFGAS_GEFont> FindFont(const CPDF_Document* doc,
                                   const ByteString& strFamilyName,
                                   bool bBold,
                                   bool bItalic,
                                   bool bStrictMatch);

  std::map<std::pair<WideString, uint32_t>, RetainPtr<CFGAS_GEFont>> font_map_;
};

#endif  // XFA_FGAS_FONT_CFGAS_PDFFONTMGR_H_
