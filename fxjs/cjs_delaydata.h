// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FXJS_CJS_DELAYDATA_H_
#define FXJS_CJS_DELAYDATA_H_

#include <stdint.h>

#include <variant>
#include <vector>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/widestring.h"
#include "fxjs/cjs_field.h"

struct CJS_DelayData {
  CJS_DelayData(FIELD_PROP prop, int idx, const WideString& name);
  ~CJS_DelayData();

  FIELD_PROP eProp;
  int nControlIndex;
  WideString sFieldName;
  std::variant<int32_t,
               bool,
               ByteString,
               CFX_FloatRect,
               std::vector<uint32_t>,
               std::vector<WideString>>
      data;
};

#endif  // FXJS_CJS_DELAYDATA_H_
