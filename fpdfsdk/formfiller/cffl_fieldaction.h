// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FORMFILLER_CFFL_FIELDACTION_H_
#define FPDFSDK_FORMFILLER_CFFL_FIELDACTION_H_

#include "core/fxcrt/widestring.h"

struct CFFL_FieldAction {
  bool modifier = false;
  bool shift = false;
  bool key_down = false;
  bool will_commit = false;
  bool field_full = false;
  bool rc = true;
  int sel_start = 0;
  int sel_end = 0;
  WideString change;
  WideString change_ex;
  WideString value;
};

#endif  // FPDFSDK_FORMFILLER_CFFL_FIELDACTION_H_
