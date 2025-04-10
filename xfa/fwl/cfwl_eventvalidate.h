// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FWL_CFWL_EVENTVALIDATE_H_
#define XFA_FWL_CFWL_EVENTVALIDATE_H_

#include "core/fxcrt/widestring.h"
#include "xfa/fwl/cfwl_event.h"

namespace pdfium {

class CFWL_EventValidate final : public CFWL_Event {
 public:
  CFWL_EventValidate(CFWL_Widget* pSrcTarget, const WideString& wsInsert);
  ~CFWL_EventValidate() override;

  WideString GetInsert() const { return insert_; }
  bool GetValidate() const { return validate_; }
  void SetValidate(bool bValidate) { validate_ = bValidate; }

 protected:
  const WideString insert_;
  bool validate_ = true;
};

}  // namespace pdfium

// TODO(crbug.com/42271761): Remove.
using pdfium::CFWL_EventValidate;

#endif  // XFA_FWL_CFWL_EVENTVALIDATE_H_
