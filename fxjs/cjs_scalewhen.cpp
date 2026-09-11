// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_scalewhen.h"

#include "fxjs/js_define.h"

const JSConstSpec CJS_ScaleWhen::ConstSpecs[] = {
    {"always", JSConstSpec::Number, 0, nullptr},
    {"never", JSConstSpec::Number, 1, nullptr},
    {"tooBig", JSConstSpec::Number, 2, nullptr},
    {"tooSmall", JSConstSpec::Number, 3, nullptr}};

// static
void CJS_ScaleWhen::DefineJSObjects(CFXJS_Engine* pEngine) {
  pEngine->DefineObj(kObjDefnId, "scaleWhen", FXJSOBJTYPE_STATIC, nullptr,
                     nullptr);
  DefineConsts(pEngine, kObjDefnId, ConstSpecs);
}
