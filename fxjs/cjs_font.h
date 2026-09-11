// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_FONT_H_
#define FXJS_CJS_FONT_H_

#include "fxjs/cjs_object.h"
#include "fxjs/js_define.h"

class CJS_Font final : public CJS_Object {
 public:
  static constexpr uint32_t kObjDefnId = kJSFontObjDefnID;
  static void DefineJSObjects(CFXJS_Engine* pEngine);

  CJS_Font() = delete;

 private:
  static const JSConstSpec ConstSpecs[];
};

#endif  // FXJS_CJS_FONT_H_
