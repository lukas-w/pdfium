// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fpdfsdk/fpdfxfa/cpdfxfa_page.h"

#include <memory>
#include <utility>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_pageimagecache.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fxcrt/check.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_context.h"
#include "fpdfsdk/fpdfxfa/cpdfxfa_widget.h"
#include "xfa/fgas/graphics/cfgas_gegraphics.h"
#include "xfa/fxfa/cxfa_ffdocview.h"
#include "xfa/fxfa/cxfa_ffpageview.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "xfa/fxfa/cxfa_ffwidgethandler.h"

namespace {

constexpr Mask<XFA_WidgetStatus> kIteratorFilter = {
    XFA_WidgetStatus::kVisible,
    XFA_WidgetStatus::kViewable,
    XFA_WidgetStatus::kFocused,
};

CXFA_FFWidget::IteratorIface* GCedWidgetIteratorForPage(
    CXFA_FFPageView* pFFPageView,
    CPDFSDK_PageView* pPageView) {
  if (!pFFPageView) {
    return nullptr;
  }

  ObservedPtr<CPDFSDK_PageView> pWatchedPageView(pPageView);
  CXFA_FFWidget::IteratorIface* pIterator =
      pFFPageView->CreateGCedTraverseWidgetIterator(kIteratorFilter);

  // Check |pPageView| again because JS may have destroyed it.
  return pWatchedPageView ? pIterator : nullptr;
}

CXFA_FFWidget::IteratorIface* GCedWidgetIteratorForAnnot(
    CXFA_FFPageView* pFFPageView,
    CPDFSDK_Annot* pSDKAnnot) {
  if (!pFFPageView) {
    return nullptr;
  }

  CPDFXFA_Widget* pXFAWidget = ToXFAWidget(pSDKAnnot);
  if (!pXFAWidget) {
    return nullptr;
  }

  ObservedPtr<CPDFSDK_Annot> pObservedAnnot(pSDKAnnot);
  CXFA_FFWidget::IteratorIface* pWidgetIterator =
      pFFPageView->CreateGCedTraverseWidgetIterator(kIteratorFilter);

  // Check |pSDKAnnot| again because JS may have destroyed it.
  if (!pObservedAnnot) {
    return nullptr;
  }

  if (pWidgetIterator->GetCurrentWidget() != pXFAWidget->GetXFAFFWidget()) {
    pWidgetIterator->SetCurrentWidget(pXFAWidget->GetXFAFFWidget());
  }

  return pWidgetIterator;
}

}  // namespace

CPDFXFA_Page::CPDFXFA_Page(CPDF_Document* document, int page_index)
    : document_(document), page_index_(page_index) {
  DCHECK(document_->GetExtension());
  DCHECK(page_index_ >= 0);
}

CPDFXFA_Page::~CPDFXFA_Page() = default;

CPDF_Page* CPDFXFA_Page::AsPDFPage() {
  return pdfpage_.Get();
}

CPDFXFA_Page* CPDFXFA_Page::AsXFAPage() {
  return this;
}

CPDF_Document* CPDFXFA_Page::GetDocument() const {
  return document_;
}

bool CPDFXFA_Page::LoadPDFPage() {
  RetainPtr<CPDF_Dictionary> dict =
      GetDocument()->GetMutablePageDictionary(page_index_);
  if (!dict) {
    return false;
  }

  if (!pdfpage_ || pdfpage_->GetDict() != dict) {
    LoadPDFPageFromDict(std::move(dict));
  }

  return true;
}

CXFA_FFPageView* CPDFXFA_Page::GetXFAPageView() const {
  auto* pContext = static_cast<CPDFXFA_Context*>(document_->GetExtension());
  CXFA_FFDocView* pXFADocView = pContext->GetXFADocView();
  return pXFADocView ? pXFADocView->GetPageView(page_index_) : nullptr;
}

bool CPDFXFA_Page::LoadPage() {
  auto* pContext = static_cast<CPDFXFA_Context*>(document_->GetExtension());
  switch (pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      return LoadPDFPage();
    case FormType::kXFAFull:
      return !!GetXFAPageView();
  }
}

void CPDFXFA_Page::LoadPDFPageFromDict(RetainPtr<CPDF_Dictionary> pPageDict) {
  DCHECK(pPageDict);
  pdfpage_ = pdfium::MakeRetain<CPDF_Page>(GetDocument(), std::move(pPageDict));
  pdfpage_->AddPageImageCache();
  pdfpage_->ParseContent();
}

float CPDFXFA_Page::GetPageWidth() const {
  CXFA_FFPageView* pPageView = GetXFAPageView();
  if (!pdfpage_ && !pPageView) {
    return 0.0f;
  }

  auto* pContext = static_cast<CPDFXFA_Context*>(document_->GetExtension());
  switch (pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      if (pdfpage_) {
        return pdfpage_->GetPageWidth();
      }
      [[fallthrough]];
    case FormType::kXFAFull:
      if (pPageView) {
        return pPageView->GetPageViewRect().width;
      }
      break;
  }

  return 0.0f;
}

float CPDFXFA_Page::GetPageHeight() const {
  CXFA_FFPageView* pPageView = GetXFAPageView();
  if (!pdfpage_ && !pPageView) {
    return 0.0f;
  }

  auto* pContext = static_cast<CPDFXFA_Context*>(document_->GetExtension());
  switch (pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      if (pdfpage_) {
        return pdfpage_->GetPageHeight();
      }
      [[fallthrough]];
    case FormType::kXFAFull:
      if (pPageView) {
        return pPageView->GetPageViewRect().height;
      }
      break;
  }

  return 0.0f;
}

std::optional<CFX_PointF> CPDFXFA_Page::DeviceToPage(
    const FX_RECT& rect,
    int rotation,
    const CFX_PointF& device_point) const {
  CXFA_FFPageView* pPageView = GetXFAPageView();
  if (!pdfpage_ && !pPageView) {
    return std::nullopt;
  }

  CFX_Matrix page2device = GetDisplayMatrixForRect(rect, rotation);
  return page2device.GetInverse().Transform(device_point);
}

std::optional<CFX_PointF> CPDFXFA_Page::PageToDevice(
    const FX_RECT& rect,
    int rotation,
    const CFX_PointF& page_point) const {
  CXFA_FFPageView* pPageView = GetXFAPageView();
  if (!pdfpage_ && !pPageView) {
    return std::nullopt;
  }

  CFX_Matrix page2device = GetDisplayMatrixForRect(rect, rotation);
  return page2device.Transform(page_point);
}

CFX_Matrix CPDFXFA_Page::GetDisplayMatrixForRect(const FX_RECT& rect,
                                                 int rotation) const {
  CXFA_FFPageView* pPageView = GetXFAPageView();
  if (!pdfpage_ && !pPageView) {
    return CFX_Matrix();
  }

  auto* pContext = static_cast<CPDFXFA_Context*>(document_->GetExtension());
  switch (pContext->GetFormType()) {
    case FormType::kNone:
    case FormType::kAcroForm:
    case FormType::kXFAForeground:
      if (pdfpage_) {
        return pdfpage_->GetDisplayMatrixForRect(rect, rotation);
      }
      [[fallthrough]];
    case FormType::kXFAFull:
      if (pPageView) {
        return pPageView->GetDisplayMatrixForRect(rect, rotation);
      }
      break;
  }

  return CFX_Matrix();
}

CPDFSDK_Annot* CPDFXFA_Page::GetNextXFAAnnot(CPDFSDK_Annot* pSDKAnnot) const {
  CXFA_FFWidget::IteratorIface* pWidgetIterator =
      GCedWidgetIteratorForAnnot(GetXFAPageView(), pSDKAnnot);
  if (!pWidgetIterator) {
    return nullptr;
  }

  return pSDKAnnot->GetPageView()->GetAnnotForFFWidget(
      pWidgetIterator->MoveToNext());
}

CPDFSDK_Annot* CPDFXFA_Page::GetPrevXFAAnnot(CPDFSDK_Annot* pSDKAnnot) const {
  CXFA_FFWidget::IteratorIface* pWidgetIterator =
      GCedWidgetIteratorForAnnot(GetXFAPageView(), pSDKAnnot);
  if (!pWidgetIterator) {
    return nullptr;
  }

  return pSDKAnnot->GetPageView()->GetAnnotForFFWidget(
      pWidgetIterator->MoveToPrevious());
}

CPDFSDK_Annot* CPDFXFA_Page::GetFirstXFAAnnot(
    CPDFSDK_PageView* page_view) const {
  CXFA_FFWidget::IteratorIface* pWidgetIterator =
      GCedWidgetIteratorForPage(GetXFAPageView(), page_view);
  if (!pWidgetIterator) {
    return nullptr;
  }

  return page_view->GetAnnotForFFWidget(pWidgetIterator->MoveToFirst());
}

CPDFSDK_Annot* CPDFXFA_Page::GetLastXFAAnnot(
    CPDFSDK_PageView* page_view) const {
  CXFA_FFWidget::IteratorIface* pWidgetIterator =
      GCedWidgetIteratorForPage(GetXFAPageView(), page_view);
  if (!pWidgetIterator) {
    return nullptr;
  }

  return page_view->GetAnnotForFFWidget(pWidgetIterator->MoveToLast());
}

int CPDFXFA_Page::HasFormFieldAtPoint(const CFX_PointF& point) const {
  CXFA_FFPageView* pPageView = GetXFAPageView();
  if (!pPageView) {
    return -1;
  }

  CXFA_FFDocView* pDocView = pPageView->GetDocView();
  if (!pDocView) {
    return -1;
  }

  CXFA_FFWidgetHandler* pWidgetHandler = pDocView->GetWidgetHandler();
  if (!pWidgetHandler) {
    return -1;
  }
  CXFA_FFPageWidgetIterator pWidgetIterator(pPageView,
                                            XFA_WidgetStatus::kViewable);

  CXFA_FFWidget* pXFAAnnot;
  while ((pXFAAnnot = pWidgetIterator.MoveToNext()) != nullptr) {
    if (pXFAAnnot->GetFormFieldType() == FormFieldType::kXFA) {
      continue;
    }

    CFX_FloatRect rcWidget = pXFAAnnot->GetWidgetRect().ToFloatRect();
    rcWidget.Inflate(1.0f, 1.0f);
    if (rcWidget.Contains(point)) {
      return static_cast<int>(pXFAAnnot->GetFormFieldType());
    }
  }

  return -1;
}

void CPDFXFA_Page::DrawFocusAnnot(CFX_RenderDevice* pDevice,
                                  CPDFSDK_Annot* pAnnot,
                                  const CFX_Matrix& mtUser2Device,
                                  const FX_RECT& rtClip) {
  CFX_RectF rectClip(rtClip);
  CFGAS_GEGraphics gs(pDevice);
  gs.SetClipRect(rectClip);

  CXFA_FFPageView* xfaView = GetXFAPageView();
  CXFA_FFPageWidgetIterator pWidgetIterator(
      xfaView, Mask<XFA_WidgetStatus>{XFA_WidgetStatus::kVisible,
                                      XFA_WidgetStatus::kViewable});

  while (true) {
    CXFA_FFWidget* pWidget = pWidgetIterator.MoveToNext();
    if (!pWidget) {
      break;
    }

    CFX_RectF rtWidgetBox = pWidget->GetBBox(CXFA_FFWidget::kDoNotDrawFocus);
    ++rtWidgetBox.width;
    ++rtWidgetBox.height;
    if (rtWidgetBox.IntersectWith(rectClip)) {
      pWidget->RenderWidget(&gs, mtUser2Device, CXFA_FFWidget::kHighlight);
    }
  }

  CPDFXFA_Widget* pXFAWidget = ToXFAWidget(pAnnot);
  if (!pXFAWidget) {
    return;
  }

  CXFA_FFDocView* docView = xfaView->GetDocView();
  if (!docView) {
    return;
  }

  docView->GetWidgetHandler()->RenderWidget(pXFAWidget->GetXFAFFWidget(), &gs,
                                            mtUser2Device, false);
}
