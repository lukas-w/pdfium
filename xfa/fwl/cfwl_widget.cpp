// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/cfwl_widget.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "core/fxcrt/check.h"
#include "v8/include/cppgc/visitor.h"
#include "xfa/fde/cfde_textout.h"
#include "xfa/fwl/cfwl_app.h"
#include "xfa/fwl/cfwl_combobox.h"
#include "xfa/fwl/cfwl_event.h"
#include "xfa/fwl/cfwl_eventmouse.h"
#include "xfa/fwl/cfwl_messagekey.h"
#include "xfa/fwl/cfwl_messagekillfocus.h"
#include "xfa/fwl/cfwl_messagemouse.h"
#include "xfa/fwl/cfwl_messagemousewheel.h"
#include "xfa/fwl/cfwl_messagesetfocus.h"
#include "xfa/fwl/cfwl_notedriver.h"
#include "xfa/fwl/cfwl_themebackground.h"
#include "xfa/fwl/cfwl_themepart.h"
#include "xfa/fwl/cfwl_themetext.h"
#include "xfa/fwl/cfwl_widgetmgr.h"
#include "xfa/fwl/ifwl_themeprovider.h"

namespace pdfium {

namespace {

constexpr float kCalcHeight = 2048.0f;
constexpr float kCalcWidth = 2048.0f;
constexpr float kCalcMultiLineDefWidth = 120.0f;

}  // namespace

CFWL_Widget::CFWL_Widget(CFWL_App* app,
                         const Properties& properties,
                         CFWL_Widget* pOuter)
    : properties_(properties),
      fwlapp_(app),
      widget_mgr_(app->GetWidgetMgr()),
      outer_(pOuter) {
  widget_mgr_->InsertWidget(outer_, this);
}

CFWL_Widget::~CFWL_Widget() = default;

void CFWL_Widget::PreFinalize() {
  CHECK(!IsLocked());  // Prefer hard stop to UaF.
  NotifyDriver();
  widget_mgr_->RemoveWidget(this);
}

void CFWL_Widget::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(adapter_iface_);
  visitor->Trace(fwlapp_);
  visitor->Trace(widget_mgr_);
  visitor->Trace(delegate_);
  visitor->Trace(outer_);
}

bool CFWL_Widget::IsForm() const {
  return false;
}

CFX_RectF CFWL_Widget::GetAutosizedWidgetRect() {
  return CFX_RectF();
}

CFX_RectF CFWL_Widget::GetWidgetRect() {
  return widget_rect_;
}

void CFWL_Widget::InflateWidgetRect(CFX_RectF& rect) {
  if (!HasBorder()) {
    return;
  }

  float fBorder = GetCXBorderSize();
  rect.Inflate(fBorder, fBorder);
}

void CFWL_Widget::SetWidgetRect(const CFX_RectF& rect) {
  widget_rect_ = rect;
}

CFX_RectF CFWL_Widget::GetClientRect() {
  return GetEdgeRect();
}

void CFWL_Widget::ModifyStyles(uint32_t dwStylesAdded,
                               uint32_t dwStylesRemoved) {
  properties_.styles_ &= ~dwStylesRemoved;
  properties_.styles_ |= dwStylesAdded;
}

void CFWL_Widget::ModifyStyleExts(uint32_t dwStyleExtsAdded,
                                  uint32_t dwStyleExtsRemoved) {
  properties_.style_exts_ &= ~dwStyleExtsRemoved;
  properties_.style_exts_ |= dwStyleExtsAdded;
}

static void NotifyHideChildWidget(CFWL_WidgetMgr* widgetMgr,
                                  CFWL_Widget* widget,
                                  CFWL_NoteDriver* noteDriver) {
  CFWL_Widget* child = widgetMgr->GetFirstChildWidget(widget);
  while (child) {
    noteDriver->NotifyTargetHide(child);
    NotifyHideChildWidget(widgetMgr, child, noteDriver);
    child = widgetMgr->GetNextSiblingWidget(child);
  }
}

void CFWL_Widget::SetStates(uint32_t dwStates) {
  properties_.states_ |= dwStates;
  if (IsVisible()) {
    return;
  }

  CFWL_NoteDriver* noteDriver = GetFWLApp()->GetNoteDriver();
  noteDriver->NotifyTargetHide(this);

  CFWL_WidgetMgr* widgetMgr = GetFWLApp()->GetWidgetMgr();
  CFWL_Widget* child = widgetMgr->GetFirstChildWidget(this);
  while (child) {
    noteDriver->NotifyTargetHide(child);
    NotifyHideChildWidget(widgetMgr, child, noteDriver);
    child = widgetMgr->GetNextSiblingWidget(child);
  }
}

void CFWL_Widget::RemoveStates(uint32_t dwStates) {
  properties_.states_ &= ~dwStates;
}

FWL_WidgetHit CFWL_Widget::HitTest(const CFX_PointF& point) {
  if (GetClientRect().Contains(point)) {
    return FWL_WidgetHit::Client;
  }
  if (HasBorder() && GetRelativeRect().Contains(point)) {
    return FWL_WidgetHit::Border;
  }
  return FWL_WidgetHit::Unknown;
}

CFX_PointF CFWL_Widget::TransformTo(CFWL_Widget* pWidget,
                                    const CFX_PointF& point) {
  CFX_SizeF szOffset;
  if (IsParent(pWidget)) {
    szOffset = GetOffsetFromParent(pWidget);
  } else {
    szOffset = pWidget->GetOffsetFromParent(this);
    szOffset.width = -szOffset.width;
    szOffset.height = -szOffset.height;
  }
  return point + CFX_PointF(szOffset.width, szOffset.height);
}

CFX_Matrix CFWL_Widget::GetMatrix() const {
  CFWL_Widget* parent = GetParent();
  std::vector<CFWL_Widget*> parents;
  while (parent) {
    parents.push_back(parent);
    parent = parent->GetParent();
  }

  CFX_Matrix matrix;
  for (size_t i = parents.size(); i >= 2; i--) {
    CFX_RectF rect = parents[i - 2]->GetWidgetRect();
    matrix.TranslatePrepend(rect.left, rect.top);
  }
  return matrix;
}

IFWL_ThemeProvider* CFWL_Widget::GetThemeProvider() const {
  return GetFWLApp()->GetThemeProvider();
}

bool CFWL_Widget::IsEnabled() const {
  return (properties_.states_ & FWL_STATE_WGT_Disabled) == 0;
}

bool CFWL_Widget::HasBorder() const {
  return !!(properties_.styles_ & FWL_STYLE_WGT_Border);
}

bool CFWL_Widget::IsVisible() const {
  return !(properties_.states_ & FWL_STATE_WGT_Invisible);
}

bool CFWL_Widget::IsOverLapper() const {
  return (properties_.styles_ & FWL_STYLE_WGT_WindowTypeMask) ==
         FWL_STYLE_WGT_OverLapper;
}

bool CFWL_Widget::IsPopup() const {
  return !!(properties_.styles_ & FWL_STYLE_WGT_Popup);
}

bool CFWL_Widget::IsChild() const {
  return !!(properties_.styles_ & FWL_STYLE_WGT_Child);
}

CFWL_Widget* CFWL_Widget::GetOutmost() const {
  CFWL_Widget* pOuter = const_cast<CFWL_Widget*>(this);
  while (pOuter->GetOuter()) {
    pOuter = pOuter->GetOuter();
  }
  return pOuter;
}

CFX_RectF CFWL_Widget::GetEdgeRect() const {
  CFX_RectF rtEdge(0, 0, widget_rect_.width, widget_rect_.height);
  if (HasBorder()) {
    rtEdge.Deflate(GetCXBorderSize(), GetCYBorderSize());
  }
  return rtEdge;
}

float CFWL_Widget::GetCXBorderSize() const {
  return GetThemeProvider()->GetCXBorderSize();
}

float CFWL_Widget::GetCYBorderSize() const {
  return GetThemeProvider()->GetCYBorderSize();
}

CFX_RectF CFWL_Widget::GetRelativeRect() const {
  return CFX_RectF(0, 0, widget_rect_.width, widget_rect_.height);
}

CFX_SizeF CFWL_Widget::CalcTextSize(const WideString& wsText, bool bMultiLine) {
  CFWL_ThemeText calPart(CFWL_ThemePart::Part::kNone, this, nullptr);
  calPart.text_ = wsText;
  if (bMultiLine) {
    calPart.tto_styles_.line_wrap_ = true;
  } else {
    calPart.tto_styles_.single_line_ = true;
  }

  calPart.tto_align_ = FDE_TextAlignment::kTopLeft;
  float fWidth = bMultiLine ? kCalcMultiLineDefWidth : kCalcWidth;
  CFX_RectF rect(0, 0, fWidth, kCalcHeight);
  GetThemeProvider()->CalcTextRect(calPart, &rect);
  return CFX_SizeF(rect.width, rect.height);
}

void CFWL_Widget::CalcTextRect(const WideString& wsText,
                               const FDE_TextStyle& dwTTOStyles,
                               FDE_TextAlignment iTTOAlign,
                               CFX_RectF* pRect) {
  CFWL_ThemeText calPart(CFWL_ThemePart::Part::kNone, this, nullptr);
  calPart.text_ = wsText;
  calPart.tto_styles_ = dwTTOStyles;
  calPart.tto_align_ = iTTOAlign;
  GetThemeProvider()->CalcTextRect(calPart, pRect);
}

void CFWL_Widget::SetGrab(bool bSet) {
  CFWL_NoteDriver* pDriver = GetFWLApp()->GetNoteDriver();
  pDriver->SetGrab(bSet ? this : nullptr);
}

void CFWL_Widget::UnregisterEventTarget() {
  CFWL_NoteDriver* pNoteDriver = GetFWLApp()->GetNoteDriver();
  pNoteDriver->UnregisterEventTarget(this);
}

void CFWL_Widget::DispatchEvent(CFWL_Event* pEvent) {
  if (outer_) {
    outer_->GetDelegate()->OnProcessEvent(pEvent);
    return;
  }
  CFWL_NoteDriver* pNoteDriver = GetFWLApp()->GetNoteDriver();
  pNoteDriver->SendEvent(pEvent);
}

void CFWL_Widget::RepaintRect(const CFX_RectF& pRect) {
  widget_mgr_->RepaintWidget(this, pRect);
}

void CFWL_Widget::DrawBackground(CFGAS_GEGraphics* pGraphics,
                                 CFWL_ThemePart::Part iPartBk,
                                 const CFX_Matrix& mtMatrix) {
  CFWL_ThemeBackground param(iPartBk, this, pGraphics);
  param.matrix_ = mtMatrix;
  param.part_rect_ = GetRelativeRect();
  GetThemeProvider()->DrawBackground(param);
}

void CFWL_Widget::DrawBorder(CFGAS_GEGraphics* pGraphics,
                             CFWL_ThemePart::Part iPartBorder,
                             const CFX_Matrix& matrix) {
  CFWL_ThemeBackground param(iPartBorder, this, pGraphics);
  param.matrix_ = matrix;
  param.part_rect_ = GetRelativeRect();
  GetThemeProvider()->DrawBackground(param);
}

void CFWL_Widget::NotifyDriver() {
  CFWL_NoteDriver* pDriver = GetFWLApp()->GetNoteDriver();
  pDriver->NotifyTargetDestroy(this);
}

CFX_SizeF CFWL_Widget::GetOffsetFromParent(CFWL_Widget* pParent) {
  if (pParent == this) {
    return CFX_SizeF();
  }

  CFX_SizeF szRet(widget_rect_.left, widget_rect_.top);
  CFWL_WidgetMgr* pWidgetMgr = GetFWLApp()->GetWidgetMgr();
  CFWL_Widget* pDstWidget = GetParent();
  while (pDstWidget && pDstWidget != pParent) {
    CFX_RectF rtDst = pDstWidget->GetWidgetRect();
    szRet += CFX_SizeF(rtDst.left, rtDst.top);
    pDstWidget = pWidgetMgr->GetParentWidget(pDstWidget);
  }
  return szRet;
}

bool CFWL_Widget::IsParent(CFWL_Widget* pParent) {
  CFWL_Widget* pUpWidget = GetParent();
  while (pUpWidget) {
    if (pUpWidget == pParent) {
      return true;
    }
    pUpWidget = pUpWidget->GetParent();
  }
  return false;
}

void CFWL_Widget::OnProcessMessage(CFWL_Message* pMessage) {
  CFWL_Widget* pWidget = pMessage->GetDstTarget();
  if (!pWidget) {
    return;
  }

  switch (pMessage->GetType()) {
    case CFWL_Message::Type::kMouse: {
      CFWL_MessageMouse* pMsgMouse = static_cast<CFWL_MessageMouse*>(pMessage);
      CFWL_EventMouse evt(pWidget, pWidget, pMsgMouse->cmd_);
      pWidget->DispatchEvent(&evt);
      break;
    }
    default:
      break;
  }
}

void CFWL_Widget::OnProcessEvent(CFWL_Event* pEvent) {}

CFWL_Widget::ScopedUpdateLock::ScopedUpdateLock(CFWL_Widget* widget)
    : widget_(widget) {
  widget_->LockUpdate();
}

CFWL_Widget::ScopedUpdateLock::~ScopedUpdateLock() {
  widget_->UnlockUpdate();
}

}  // namespace pdfium
