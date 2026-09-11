// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_highlight.h"

#include "fxjs/js_define.h"

const JSConstSpec CJS_Highlight::ConstSpecs[] = {
    {"n", JSConstSpec::String, 0, "none"},
    {"i", JSConstSpec::String, 0, "invert"},
    {"p", JSConstSpec::String, 0, "push"},
    {"o", JSConstSpec::String, 0, "outline"}};

// static
void CJS_Highlight::DefineJSObjects(CFXJS_Engine* pEngine) {
  pEngine->DefineObj(kObjDefnId, "highlight", FXJSOBJTYPE_STATIC, nullptr,
                     nullptr);
  DefineConsts(pEngine, kObjDefnId, ConstSpecs);
}
