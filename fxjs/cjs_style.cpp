// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_style.h"

#include "fxjs/js_define.h"

const JSConstSpec CJS_Style::ConstSpecs[] = {
    {"ch", JSConstSpec::String, 0, "check"},
    {"cr", JSConstSpec::String, 0, "cross"},
    {"di", JSConstSpec::String, 0, "diamond"},
    {"ci", JSConstSpec::String, 0, "circle"},
    {"st", JSConstSpec::String, 0, "star"},
    {"sq", JSConstSpec::String, 0, "square"}};

// static
void CJS_Style::DefineJSObjects(CFXJS_Engine* pEngine) {
  pEngine->DefineObj(kObjDefnId, "style", FXJSOBJTYPE_STATIC, nullptr, nullptr);
  DefineConsts(pEngine, kObjDefnId, ConstSpecs);
}
