// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_FPDFXFA_CPDFXFA_WIDGET_H_
#define FPDFSDK_FPDFXFA_CPDFXFA_WIDGET_H_

#include "core/fxcrt/fx_coordinates.h"
#include "fpdfsdk/cpdfsdk_annot.h"
#include "v8/include/cppgc/persistent.h"
#include "xfa/fxfa/cxfa_ffwidget.h"

class CPDFSDK_PageView;
class CXFA_FFDocView;
class CXFA_FFWidgetHandler;

class CPDFXFA_Widget final : public CPDFSDK_Annot,
                             CPDFSDK_Annot::UnsafeInputHandlers {
 public:
  CPDFXFA_Widget(CXFA_FFWidget* pXFAFFWidget, CPDFSDK_PageView* pPageView);
  ~CPDFXFA_Widget() override;

  // CPDFSDK_Annot:
  CPDFXFA_Widget* AsXFAWidget() override;
  UnsafeInputHandlers* GetUnsafeInputHandlers() override;
  CPDF_Annot::Subtype GetAnnotSubtype() const override;
  CFX_FloatRect GetRect() const override;
  bool DoHitTest(const CFX_PointF& point) override;
  CFX_FloatRect GetViewBBox() override;
  bool CanUndo() override;
  bool CanRedo() override;
  bool Undo() override;
  bool Redo() override;

  CXFA_FFWidget* GetXFAFFWidget() const { return m_pXFAFFWidget.Get(); }

  bool OnChangedFocus();

 private:
  // CPDFSDK_Annot::UnsafeInputHandlers:
  void OnMouseEnter(Mask<FWL_EVENTFLAG> nFlags) override;
  void OnMouseExit(Mask<FWL_EVENTFLAG> nFlags) override;
  bool OnLButtonDown(Mask<FWL_EVENTFLAG> nFlags,
                     const CFX_PointF& point) override;
  bool OnLButtonUp(Mask<FWL_EVENTFLAG> nFlags,
                   const CFX_PointF& point) override;
  bool OnLButtonDblClk(Mask<FWL_EVENTFLAG> nFlags,
                       const CFX_PointF& point) override;
  bool OnMouseMove(Mask<FWL_EVENTFLAG> nFlags,
                   const CFX_PointF& point) override;
  bool OnMouseWheel(Mask<FWL_EVENTFLAG> nFlags,
                    const CFX_PointF& point,
                    const CFX_Vector& delta) override;
  bool OnRButtonDown(Mask<FWL_EVENTFLAG> nFlags,
                     const CFX_PointF& point) override;
  bool OnRButtonUp(Mask<FWL_EVENTFLAG> nFlags,
                   const CFX_PointF& point) override;

  CXFA_FFDocView* GetDocView();
  CXFA_FFWidgetHandler* GetWidgetHandler();

  cppgc::Persistent<CXFA_FFWidget> const m_pXFAFFWidget;
};

#endif  // FPDFSDK_FPDFXFA_CPDFXFA_WIDGET_H_
