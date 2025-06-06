// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef FPDFSDK_CPDFSDK_PAGEVIEW_H_
#define FPDFSDK_CPDFSDK_PAGEVIEW_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fxcrt/mask.h"
#include "core/fxcrt/unowned_ptr.h"
#include "fpdfsdk/cpdfsdk_annot.h"

class CFX_RenderDevice;
class CPDF_AnnotList;
class CPDF_RenderOptions;
class CPDFSDK_FormFillEnvironment;
class CPDFSDK_InteractiveForm;

#ifdef PDF_ENABLE_XFA
class CPDFXFA_Page;
class CXFA_FFWidget;
#endif  // PDF_ENABLE_XFA

class CPDFSDK_PageView final : public CPDF_Page::View {
 public:
  CPDFSDK_PageView(CPDFSDK_FormFillEnvironment* pFormFillEnv, IPDF_Page* page);
  ~CPDFSDK_PageView();

  // CPDF_Page::View:
  void ClearPage(CPDF_Page* pPage) override;

  void PageView_OnDraw(CFX_RenderDevice* pDevice,
                       const CFX_Matrix& mtUser2Device,
                       CPDF_RenderOptions* pOptions,
                       const FX_RECT& pClip);

  void LoadFXAnnots();
  CPDFSDK_Annot* GetFocusAnnot();
  CPDFSDK_Annot* GetNextAnnot(CPDFSDK_Annot* pAnnot);
  CPDFSDK_Annot* GetPrevAnnot(CPDFSDK_Annot* pAnnot);
  CPDFSDK_Annot* GetFirstFocusableAnnot();
  CPDFSDK_Annot* GetLastFocusableAnnot();
  bool IsValidAnnot(const CPDF_Annot* p) const;
  bool IsValidSDKAnnot(const CPDFSDK_Annot* p) const;

  std::vector<CPDFSDK_Annot*> GetAnnotList() const;
  CPDFSDK_Annot* GetAnnotByDict(const CPDF_Dictionary* dict);

#ifdef PDF_ENABLE_XFA
  CPDFSDK_Annot* AddAnnotForFFWidget(CXFA_FFWidget* pWidget);
  void DeleteAnnotForFFWidget(CXFA_FFWidget* pWidget);
  CPDFSDK_Annot* GetAnnotForFFWidget(CXFA_FFWidget* pWidget);
  IPDF_Page* GetXFAPage();
#endif  // PDF_ENABLE_XFA

  CPDF_Page* GetPDFPage() const;
  CPDF_Document* GetPDFDocument();
  CPDFSDK_FormFillEnvironment* GetFormFillEnv() const { return form_fill_env_; }

  WideString GetFocusedFormText();
  WideString GetSelectedText();
  void ReplaceAndKeepSelection(const WideString& text);
  void ReplaceSelection(const WideString& text);
  bool SelectAllText();

  bool CanUndo();
  bool CanRedo();
  bool Undo();
  bool Redo();

  bool OnFocus(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnLButtonDown(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnLButtonUp(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnLButtonDblClk(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnRButtonDown(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnRButtonUp(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnChar(uint32_t nChar, Mask<FWL_EVENTFLAG> nFlags);
  bool OnKeyDown(FWL_VKEYCODE nKeyCode, Mask<FWL_EVENTFLAG> nFlags);
  bool OnMouseMove(Mask<FWL_EVENTFLAG> nFlags, const CFX_PointF& point);
  bool OnMouseWheel(Mask<FWL_EVENTFLAG> nFlags,
                    const CFX_PointF& point,
                    const CFX_Vector& delta);

  bool SetIndexSelected(int index, bool selected);
  bool IsIndexSelected(int index);

  const CFX_Matrix& GetCurrentMatrix() const { return matrix_; }
  void UpdateRects(const std::vector<CFX_FloatRect>& rects);
  void UpdateView(CPDFSDK_Annot* pAnnot);

  int GetPageIndex() const;

  void SetValid(bool bValid) { valid_ = bValid; }
  bool IsValid() const { return valid_; }
  bool IsLocked() const { return locked_; }
  void SetBeingDestroyed() { being_destroyed_ = true; }
  bool IsBeingDestroyed() const { return being_destroyed_; }

 private:
#ifdef PDF_ENABLE_XFA
  CPDFXFA_Page* XFAPageIfNotBackedByPDFPage();
#endif

  std::unique_ptr<CPDFSDK_Annot> NewAnnot(CPDF_Annot* annot);

  CPDFSDK_InteractiveForm* GetInteractiveForm() const;
  CPDFSDK_Annot* GetFXAnnotAtPoint(const CFX_PointF& point);
  CPDFSDK_Annot* GetFXWidgetAtPoint(const CFX_PointF& point);

  int GetPageIndexForStaticPDF() const;

  void EnterWidget(ObservedPtr<CPDFSDK_Annot>& pAnnot,
                   Mask<FWL_EVENTFLAG> nFlags);
  void ExitWidget(bool callExitCallback, Mask<FWL_EVENTFLAG> nFlags);

  CFX_Matrix matrix_;
  UnownedPtr<IPDF_Page> const page_;
  std::unique_ptr<CPDF_AnnotList> annot_list_;
  std::vector<std::unique_ptr<CPDFSDK_Annot>> sdkannot_array_;
  UnownedPtr<CPDFSDK_FormFillEnvironment> const form_fill_env_;
  ObservedPtr<CPDFSDK_Annot> capture_widget_;
  bool on_widget_ = false;
  bool valid_ = false;
  bool locked_ = false;
  bool being_destroyed_ = false;
};

#endif  // FPDFSDK_CPDFSDK_PAGEVIEW_H_
