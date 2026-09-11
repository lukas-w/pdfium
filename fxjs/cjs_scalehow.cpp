// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_scalehow.h"

#include "fxjs/js_define.h"

const JSConstSpec CJS_ScaleHow::ConstSpecs[] = {
    {"proportional", JSConstSpec::Number, 0, 0},
    {"anamorphic", JSConstSpec::Number, 1, 0}};

// static
void CJS_ScaleHow::DefineJSObjects(CFXJS_Engine* pEngine) {
  pEngine->DefineObj(kObjDefnId, "scaleHow", FXJSOBJTYPE_STATIC, nullptr,
                     nullptr);
  DefineConsts(pEngine, kObjDefnId, ConstSpecs);
}
