// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXFA_CXFA_FFNUMERICEDIT_H_
#define XFA_FXFA_CXFA_FFNUMERICEDIT_H_

#include "core/fxcrt/widestring.h"
#include "xfa/fxfa/cxfa_fftextedit.h"

namespace pdfium {
class CFWL_Widget;
}  // namespace pdfium

class CXFA_FFNumericEdit final : public CXFA_FFTextEdit {
 public:
  explicit CXFA_FFNumericEdit(CXFA_Node* pNode);
  ~CXFA_FFNumericEdit() override;

  // CXFA_FFTextEdit
  bool LoadWidget() override;
  void UpdateWidgetProperty() override;
  void OnProcessEvent(pdfium::CFWL_Event* pEvent) override;

 private:
  bool OnValidate(CFWL_Widget* pWidget, const WideString& wsText);
};

#endif  // XFA_FXFA_CXFA_FFNUMERICEDIT_H_
