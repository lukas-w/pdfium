// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "fxjs/cjs_document.h"

#include <stdint.h>

#include <utility>
#include <vector>

#include "constants/access_permissions.h"
#include "core/fpdfapi/page/cpdf_pageimagecache.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfdoc/cpdf_interactiveform.h"
#include "core/fpdfdoc/cpdf_nametree.h"
#include "core/fxcrt/check.h"
#include "core/fxcrt/span.h"
#include "fpdfsdk/cpdfsdk_annotiteration.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_interactiveform.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fxjs/cjs_annot.h"
#include "fxjs/cjs_app.h"
#include "fxjs/cjs_delaydata.h"
#include "fxjs/cjs_event_context.h"
#include "fxjs/cjs_field.h"
#include "fxjs/cjs_icon.h"
#include "fxjs/js_resources.h"
#include "v8/include/v8-container.h"

const JSPropertySpec CJS_Document::PropertySpecs[] = {
    {"ADBE", get_ADBE_static, set_ADBE_static},
    {"author", get_author_static, set_author_static},
    {"baseURL", get_base_URL_static, set_base_URL_static},
    {"bookmarkRoot", get_bookmark_root_static, set_bookmark_root_static},
    {"calculate", get_calculate_static, set_calculate_static},
    {"Collab", get_collab_static, set_collab_static},
    {"creationDate", get_creation_date_static, set_creation_date_static},
    {"creator", get_creator_static, set_creator_static},
    {"delay", get_delay_static, set_delay_static},
    {"dirty", get_dirty_static, set_dirty_static},
    {"documentFileName", get_document_file_name_static,
     set_document_file_name_static},
    {"external", get_external_static, set_external_static},
    {"filesize", get_filesize_static, set_filesize_static},
    {"icons", get_icons_static, set_icons_static},
    {"info", get_info_static, set_info_static},
    {"keywords", get_keywords_static, set_keywords_static},
    {"layout", get_layout_static, set_layout_static},
    {"media", get_media_static, set_media_static},
    {"modDate", get_mod_date_static, set_mod_date_static},
    {"mouseX", get_mouse_x_static, set_mouse_x_static},
    {"mouseY", get_mouse_y_static, set_mouse_y_static},
    {"numFields", get_num_fields_static, set_num_fields_static},
    {"numPages", get_num_pages_static, set_num_pages_static},
    {"pageNum", get_page_num_static, set_page_num_static},
    {"pageWindowRect", get_page_window_rect_static,
     set_page_window_rect_static},
    {"path", get_path_static, set_path_static},
    {"producer", get_producer_static, set_producer_static},
    {"subject", get_subject_static, set_subject_static},
    {"title", get_title_static, set_title_static},
    {"URL", get_URL_static, set_URL_static},
    {"zoom", get_zoom_static, set_zoom_static},
    {"zoomType", get_zoom_type_static, set_zoom_type_static}};

const JSMethodSpec CJS_Document::MethodSpecs[] = {
    {"addAnnot", addAnnot_static},
    {"addField", addField_static},
    {"addLink", addLink_static},
    {"addIcon", addIcon_static},
    {"calculateNow", calculateNow_static},
    {"closeDoc", closeDoc_static},
    {"createDataObject", createDataObject_static},
    {"deletePages", deletePages_static},
    {"exportAsText", exportAsText_static},
    {"exportAsFDF", exportAsFDF_static},
    {"exportAsXFDF", exportAsXFDF_static},
    {"extractPages", extractPages_static},
    {"getAnnot", getAnnot_static},
    {"getAnnots", getAnnots_static},
    {"getAnnot3D", getAnnot3D_static},
    {"getAnnots3D", getAnnots3D_static},
    {"getField", getField_static},
    {"getIcon", getIcon_static},
    {"getLinks", getLinks_static},
    {"getNthFieldName", getNthFieldName_static},
    {"getOCGs", getOCGs_static},
    {"getPageBox", getPageBox_static},
    {"getPageNthWord", getPageNthWord_static},
    {"getPageNthWordQuads", getPageNthWordQuads_static},
    {"getPageNumWords", getPageNumWords_static},
    {"getPrintParams", getPrintParams_static},
    {"getURL", getURL_static},
    {"gotoNamedDest", gotoNamedDest_static},
    {"importAnFDF", importAnFDF_static},
    {"importAnXFDF", importAnXFDF_static},
    {"importTextData", importTextData_static},
    {"insertPages", insertPages_static},
    {"mailDoc", mailDoc_static},
    {"mailForm", mailForm_static},
    {"print", print_static},
    {"removeField", removeField_static},
    {"replacePages", replacePages_static},
    {"resetForm", resetForm_static},
    {"removeIcon", removeIcon_static},
    {"saveAs", saveAs_static},
    {"submitForm", submitForm_static},
    {"syncAnnotScan", syncAnnotScan_static}};

uint32_t CJS_Document::ObjDefnID = 0;
const char CJS_Document::kName[] = "Document";

// static
uint32_t CJS_Document::GetObjDefnID() {
  return ObjDefnID;
}

// static
void CJS_Document::DefineJSObjects(CFXJS_Engine* pEngine) {
  ObjDefnID = pEngine->DefineObj(CJS_Document::kName, FXJSOBJTYPE_GLOBAL,
                                 JSConstructor<CJS_Document>, JSDestructor);
  DefineProps(pEngine, ObjDefnID, PropertySpecs);
  DefineMethods(pEngine, ObjDefnID, MethodSpecs);
}

CJS_Document::CJS_Document(v8::Local<v8::Object> pObject, CJS_Runtime* pRuntime)
    : CJS_Object(pObject, pRuntime) {
  SetFormFillEnv(GetRuntime()->GetFormFillEnv());
}

CJS_Document::~CJS_Document() = default;

// The total number of fields in document.
CJS_Result CJS_Document::get_num_fields(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDF_InteractiveForm* pPDFForm = GetCoreInteractiveForm();
  return CJS_Result::Success(pRuntime->NewNumber(
      static_cast<int>(pPDFForm->CountFields(WideString()))));
}

CJS_Result CJS_Document::set_num_fields(CJS_Runtime* pRuntime,
                                        v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::get_dirty(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  return CJS_Result::Success(
      pRuntime->NewBoolean(!!form_fill_env_->GetChangeMark()));
}

CJS_Result CJS_Document::set_dirty(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  pRuntime->ToBoolean(vp) ? form_fill_env_->SetChangeMark()
                          : form_fill_env_->ClearChangeMark();
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_ADBE(CJS_Runtime* pRuntime) {
  return CJS_Result::Success(pRuntime->NewUndefined());
}

CJS_Result CJS_Document::set_ADBE(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_page_num(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDFSDK_PageView* pPageView = form_fill_env_->GetCurrentView();
  if (!pPageView) {
    return CJS_Result::Success(pRuntime->NewUndefined());
  }

  return CJS_Result::Success(pRuntime->NewNumber(pPageView->GetPageIndex()));
}

CJS_Result CJS_Document::set_page_num(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  int iPageCount = form_fill_env_->GetPageCount();
  int iPageNum = pRuntime->ToInt32(vp);
  if (iPageNum >= 0 && iPageNum < iPageCount) {
    form_fill_env_->JS_docgotoPage(iPageNum);
  } else if (iPageNum >= iPageCount) {
    form_fill_env_->JS_docgotoPage(iPageCount - 1);
  } else if (iPageNum < 0) {
    form_fill_env_->JS_docgotoPage(0);
  }

  return CJS_Result::Success();
}

CJS_Result CJS_Document::addAnnot(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  // Not supported, but do not return an error.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::addField(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  // Not supported, but do not return an error.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::exportAsText(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported, but do not return an error.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::exportAsFDF(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported, but do not return an error.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::exportAsXFDF(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported, but do not return an error.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getField(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  if (params.empty()) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }

  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  WideString wideName = pRuntime->ToWideString(params[0]);
  CPDF_InteractiveForm* pPDFForm = GetCoreInteractiveForm();
  if (pPDFForm->CountFields(wideName) <= 0) {
    return CJS_Result::Success(pRuntime->NewUndefined());
  }

  v8::Local<v8::Object> pFieldObj = pRuntime->NewFXJSBoundObject(
      CJS_Field::GetObjDefnID(), FXJSOBJTYPE_DYNAMIC);
  if (pFieldObj.IsEmpty()) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  auto* pJSField = static_cast<CJS_Field*>(
      CFXJS_Engine::GetBinding(pRuntime->GetIsolate(), pFieldObj));
  if (!pJSField) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  pJSField->AttachField(this, wideName);
  return CJS_Result::Success(pJSField->ToV8Object());
}

// Gets the name of the nth field in the document
CJS_Result CJS_Document::getNthFieldName(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (params.size() != 1) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  int nIndex = pRuntime->ToInt32(params[0]);
  if (nIndex < 0) {
    return CJS_Result::Failure(JSMessage::kValueError);
  }

  CPDF_InteractiveForm* pPDFForm = GetCoreInteractiveForm();
  CPDF_FormField* pField = pPDFForm->GetField(nIndex, WideString());
  if (!pField) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }
  return CJS_Result::Success(
      pRuntime->NewString(pField->GetFullName().AsStringView()));
}

CJS_Result CJS_Document::importAnFDF(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::importAnXFDF(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::importTextData(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::mailDoc(CJS_Runtime* pRuntime,
                                 pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  v8::LocalVector<v8::Value> newParams = ExpandKeywordParams(
      pRuntime, params, 6, "bUI", "cTo", "cCc", "cBcc", "cSubject", "cMsg");

  bool bUI = true;
  if (IsExpandedParamKnown(newParams[0])) {
    bUI = pRuntime->ToBoolean(newParams[0]);
  }

  WideString cTo;
  if (IsExpandedParamKnown(newParams[1])) {
    cTo = pRuntime->ToWideString(newParams[1]);
  }

  WideString cCc;
  if (IsExpandedParamKnown(newParams[2])) {
    cCc = pRuntime->ToWideString(newParams[2]);
  }

  WideString cBcc;
  if (IsExpandedParamKnown(newParams[3])) {
    cBcc = pRuntime->ToWideString(newParams[3]);
  }

  WideString cSubject;
  if (IsExpandedParamKnown(newParams[4])) {
    cSubject = pRuntime->ToWideString(newParams[4]);
  }

  WideString cMsg;
  if (IsExpandedParamKnown(newParams[5])) {
    cMsg = pRuntime->ToWideString(newParams[5]);
  }

  pRuntime->BeginBlock();
  form_fill_env_->JS_docmailForm(pdfium::span<const uint8_t>(), bUI, cTo,
                                 cSubject, cCc, cBcc, cMsg);
  pRuntime->EndBlock();
  return CJS_Result::Success();
}

// exports the form data and mails the resulting fdf file as an attachment to
// all recipients.
// comment: need reader supports
CJS_Result CJS_Document::mailForm(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  using pdfium::access_permissions::kExtractForAccessibility;
  if (!form_fill_env_->HasPermissions(kExtractForAccessibility)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  CPDFSDK_InteractiveForm* pInteractiveForm = GetSDKInteractiveForm();
  ByteString sTextBuf = pInteractiveForm->ExportFormToFDFTextBuf();
  if (sTextBuf.IsEmpty()) {
    return CJS_Result::Failure(WideString::FromASCII("Bad FDF format."));
  }

  v8::LocalVector<v8::Value> newParams = ExpandKeywordParams(
      pRuntime, params, 6, "bUI", "cTo", "cCc", "cBcc", "cSubject", "cMsg");

  bool bUI = true;
  if (IsExpandedParamKnown(newParams[0])) {
    bUI = pRuntime->ToBoolean(newParams[0]);
  }

  WideString cTo;
  if (IsExpandedParamKnown(newParams[1])) {
    cTo = pRuntime->ToWideString(newParams[1]);
  }

  WideString cCc;
  if (IsExpandedParamKnown(newParams[2])) {
    cCc = pRuntime->ToWideString(newParams[2]);
  }

  WideString cBcc;
  if (IsExpandedParamKnown(newParams[3])) {
    cBcc = pRuntime->ToWideString(newParams[3]);
  }

  WideString cSubject;
  if (IsExpandedParamKnown(newParams[4])) {
    cSubject = pRuntime->ToWideString(newParams[4]);
  }

  WideString cMsg;
  if (IsExpandedParamKnown(newParams[5])) {
    cMsg = pRuntime->ToWideString(newParams[5]);
  }

  pRuntime->BeginBlock();
  form_fill_env_->JS_docmailForm(sTextBuf.unsigned_span(), bUI, cTo, cSubject,
                                 cCc, cBcc, cMsg);
  pRuntime->EndBlock();
  return CJS_Result::Success();
}

CJS_Result CJS_Document::print(CJS_Runtime* pRuntime,
                               pdfium::span<v8::Local<v8::Value>> params) {
  v8::LocalVector<v8::Value> newParams = ExpandKeywordParams(
      pRuntime, params, 8, "bUI", "nStart", "nEnd", "bSilent", "bShrinkToFit",
      "bPrintAsImage", "bReverse", "bAnnotations");

  bool bUI = true;
  if (IsExpandedParamKnown(newParams[0])) {
    bUI = pRuntime->ToBoolean(newParams[0]);
  }

  int nStart = 0;
  if (IsExpandedParamKnown(newParams[1])) {
    nStart = pRuntime->ToInt32(newParams[1]);
  }

  int nEnd = 0;
  if (IsExpandedParamKnown(newParams[2])) {
    nEnd = pRuntime->ToInt32(newParams[2]);
  }

  bool bSilent = false;
  if (IsExpandedParamKnown(newParams[3])) {
    bSilent = pRuntime->ToBoolean(newParams[3]);
  }

  bool bShrinkToFit = false;
  if (IsExpandedParamKnown(newParams[4])) {
    bShrinkToFit = pRuntime->ToBoolean(newParams[4]);
  }

  bool bPrintAsImage = false;
  if (IsExpandedParamKnown(newParams[5])) {
    bPrintAsImage = pRuntime->ToBoolean(newParams[5]);
  }

  bool bReverse = false;
  if (IsExpandedParamKnown(newParams[6])) {
    bReverse = pRuntime->ToBoolean(newParams[6]);
  }

  bool bAnnotations = false;
  if (IsExpandedParamKnown(newParams[7])) {
    bAnnotations = pRuntime->ToBoolean(newParams[7]);
  }

  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CJS_EventContext* pHandler = pRuntime->GetCurrentEventContext();
  if (!pHandler->IsUserGesture()) {
    return CJS_Result::Failure(JSMessage::kUserGestureRequiredError);
  }

  form_fill_env_->JS_docprint(bUI, nStart, nEnd, bSilent, bShrinkToFit,
                              bPrintAsImage, bReverse, bAnnotations);
  return CJS_Result::Success();
}

// removes the specified field from the document.
// comment:
// note: if the filed name is not rational, adobe is dumb for it.
CJS_Result CJS_Document::removeField(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (params.size() != 1) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  if (!form_fill_env_->HasPermissions(
          pdfium::access_permissions::kModifyContent |
          pdfium::access_permissions::kModifyAnnotation)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  WideString sFieldName = pRuntime->ToWideString(params[0]);
  CPDFSDK_InteractiveForm* pInteractiveForm = GetSDKInteractiveForm();
  std::vector<ObservedPtr<CPDFSDK_Widget>> widgets;
  pInteractiveForm->GetWidgets(sFieldName, &widgets);
  if (widgets.empty()) {
    return CJS_Result::Success();
  }

  for (const auto& pWidget : widgets) {
    if (!pWidget) {
      continue;
    }

    IPDF_Page* pPage = pWidget->GetPage();
    DCHECK(pPage);

    // If there is currently no pageview associated with the page being used
    // do not create one. We may be in the process of tearing down the document
    // and creating a new pageview at this point will cause bad things.
    CPDFSDK_PageView* pPageView = form_fill_env_->GetPageView(pPage);
    if (!pPageView) {
      continue;
    }

    CFX_FloatRect rcAnnot = pWidget->GetRect();
    rcAnnot.Inflate(1.0f, 1.0f, 1.0f, 1.0f);

    std::vector<CFX_FloatRect> aRefresh(1, rcAnnot);
    pPageView->UpdateRects(aRefresh);
  }
  form_fill_env_->SetChangeMark();
  return CJS_Result::Success();
}

// reset filed values within a document.
// comment:
// note: if the fields names r not rational, aodbe is dumb for it.

CJS_Result CJS_Document::resetForm(CJS_Runtime* pRuntime,
                                   pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  if (!form_fill_env_->HasPermissions(
          pdfium::access_permissions::kModifyContent |
          pdfium::access_permissions::kModifyAnnotation |
          pdfium::access_permissions::kFillForm)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  CPDF_InteractiveForm* pPDFForm = GetCoreInteractiveForm();
  if (params.empty()) {
    pPDFForm->ResetForm();
    form_fill_env_->SetChangeMark();
    return CJS_Result::Success();
  }

  v8::Local<v8::Array> array;
  if (params[0]->IsString()) {
    array = pRuntime->NewArray();
    pRuntime->PutArrayElement(array, 0, params[0]);
  } else {
    array = pRuntime->ToArray(params[0]);
  }

  std::vector<CPDF_FormField*> aFields;
  for (size_t i = 0; i < pRuntime->GetArrayLength(array); ++i) {
    WideString swVal =
        pRuntime->ToWideString(pRuntime->GetArrayElement(array, i));
    const size_t jsz = pPDFForm->CountFields(swVal);
    for (size_t j = 0; j < jsz; ++j) {
      aFields.push_back(pPDFForm->GetField(j, swVal));
    }
  }

  if (!aFields.empty()) {
    pPDFForm->ResetForm(aFields, true);
    form_fill_env_->SetChangeMark();
  }

  return CJS_Result::Success();
}

CJS_Result CJS_Document::saveAs(CJS_Runtime* pRuntime,
                                pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::syncAnnotScan(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::submitForm(CJS_Runtime* pRuntime,
                                    pdfium::span<v8::Local<v8::Value>> params) {
  size_t nSize = params.size();
  if (nSize < 1) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CJS_EventContext* pHandler = pRuntime->GetCurrentEventContext();
  if (!pHandler->IsUserGesture()) {
    return CJS_Result::Failure(JSMessage::kUserGestureRequiredError);
  }

  v8::Local<v8::Array> aFields;
  WideString strURL;
  bool bFDF = true;
  bool bEmpty = false;
  if (params[0]->IsString()) {
    strURL = pRuntime->ToWideString(params[0]);
    if (nSize > 1) {
      bFDF = pRuntime->ToBoolean(params[1]);
    }
    if (nSize > 2) {
      bEmpty = pRuntime->ToBoolean(params[2]);
    }
    if (nSize > 3) {
      aFields = pRuntime->ToArray(params[3]);
    }
  } else if (params[0]->IsObject()) {
    v8::Local<v8::Object> pObj = pRuntime->ToObject(params[0]);
    v8::Local<v8::Value> pValue = pRuntime->GetObjectProperty(pObj, "cURL");
    if (!pValue.IsEmpty()) {
      strURL = pRuntime->ToWideString(pValue);
    }

    bFDF = pRuntime->ToBoolean(pRuntime->GetObjectProperty(pObj, "bFDF"));
    bEmpty = pRuntime->ToBoolean(pRuntime->GetObjectProperty(pObj, "bEmpty"));
    aFields = pRuntime->ToArray(pRuntime->GetObjectProperty(pObj, "aFields"));
  }

  CPDF_InteractiveForm* pPDFForm = GetCoreInteractiveForm();
  if (pRuntime->GetArrayLength(aFields) == 0 && bEmpty) {
    if (pPDFForm->CheckRequiredFields(nullptr, true)) {
      pRuntime->BeginBlock();
      GetSDKInteractiveForm()->SubmitForm(strURL);
      pRuntime->EndBlock();
    }
    return CJS_Result::Success();
  }

  std::vector<CPDF_FormField*> fieldObjects;
  for (size_t i = 0; i < pRuntime->GetArrayLength(aFields); ++i) {
    WideString sName =
        pRuntime->ToWideString(pRuntime->GetArrayElement(aFields, i));
    const size_t jsz = pPDFForm->CountFields(sName);
    for (size_t j = 0; j < jsz; ++j) {
      CPDF_FormField* pField = pPDFForm->GetField(j, sName);
      if (!bEmpty && pField->GetValue().IsEmpty()) {
        continue;
      }

      fieldObjects.push_back(pField);
    }
  }

  if (pPDFForm->CheckRequiredFields(&fieldObjects, true)) {
    pRuntime->BeginBlock();
    GetSDKInteractiveForm()->SubmitFields(strURL, fieldObjects, true, !bFDF);
    pRuntime->EndBlock();
  }
  return CJS_Result::Success();
}

void CJS_Document::SetFormFillEnv(CPDFSDK_FormFillEnvironment* pFormFillEnv) {
  form_fill_env_.Reset(pFormFillEnv);
}

CJS_Result CJS_Document::get_bookmark_root(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_bookmark_root(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_author(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "Author");
}

CJS_Result CJS_Document::set_author(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_info(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  RetainPtr<const CPDF_Dictionary> dict =
      form_fill_env_->GetPDFDocument()->GetInfo();
  if (!dict) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  WideString cwAuthor = dict->GetUnicodeTextFor("Author");
  WideString cwTitle = dict->GetUnicodeTextFor("Title");
  WideString cwSubject = dict->GetUnicodeTextFor("Subject");
  WideString cwKeywords = dict->GetUnicodeTextFor("Keywords");
  WideString cwCreator = dict->GetUnicodeTextFor("Creator");
  WideString cwProducer = dict->GetUnicodeTextFor("Producer");
  WideString cwCreationDate = dict->GetUnicodeTextFor("CreationDate");
  WideString cwModDate = dict->GetUnicodeTextFor("ModDate");
  WideString cwTrapped = dict->GetUnicodeTextFor("Trapped");

  v8::Local<v8::Object> pObj = pRuntime->NewObject();
  pRuntime->PutObjectProperty(pObj, "Author",
                              pRuntime->NewString(cwAuthor.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "Title",
                              pRuntime->NewString(cwTitle.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "Subject",
                              pRuntime->NewString(cwSubject.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "Keywords",
                              pRuntime->NewString(cwKeywords.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "Creator",
                              pRuntime->NewString(cwCreator.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "Producer",
                              pRuntime->NewString(cwProducer.AsStringView()));
  pRuntime->PutObjectProperty(
      pObj, "CreationDate", pRuntime->NewString(cwCreationDate.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "ModDate",
                              pRuntime->NewString(cwModDate.AsStringView()));
  pRuntime->PutObjectProperty(pObj, "Trapped",
                              pRuntime->NewString(cwTrapped.AsStringView()));

  // PutObjectProperty() calls below may re-enter JS and change info dict.
  CPDF_DictionaryLocker locker(ToDictionary(dict->Clone()));
  for (const auto& it : locker) {
    const ByteString& bsKey = it.first;
    const RetainPtr<CPDF_Object>& pValueObj = it.second;
    if (pValueObj->IsString() || pValueObj->IsName()) {
      pRuntime->PutObjectProperty(
          pObj, bsKey.AsStringView(),
          pRuntime->NewString(pValueObj->GetUnicodeText().AsStringView()));
    } else if (pValueObj->IsNumber()) {
      pRuntime->PutObjectProperty(pObj, bsKey.AsStringView(),
                                  pRuntime->NewNumber(pValueObj->GetNumber()));
    } else if (pValueObj->IsBoolean()) {
      pRuntime->PutObjectProperty(
          pObj, bsKey.AsStringView(),
          pRuntime->NewBoolean(!!pValueObj->GetInteger()));
    }
  }
  return CJS_Result::Success(pObj);
}

CJS_Result CJS_Document::set_info(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::GetPropertyInternal(CJS_Runtime* pRuntime,
                                             ByteStringView property_name) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  RetainPtr<CPDF_Dictionary> dict = form_fill_env_->GetPDFDocument()->GetInfo();
  if (!dict) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  return CJS_Result::Success(pRuntime->NewString(
      dict->GetUnicodeTextFor(property_name).AsStringView()));
}

CJS_Result CJS_Document::get_creation_date(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "CreationDate");
}

CJS_Result CJS_Document::set_creation_date(CJS_Runtime* pRuntime,
                                           v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_creator(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "Creator");
}

CJS_Result CJS_Document::set_creator(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_delay(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }
  return CJS_Result::Success(pRuntime->NewBoolean(delay_));
}

CJS_Result CJS_Document::set_delay(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  using pdfium::access_permissions::kModifyContent;
  if (!form_fill_env_->HasPermissions(kModifyContent)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  delay_ = pRuntime->ToBoolean(vp);
  if (delay_) {
    delay_data_.clear();
    return CJS_Result::Success();
  }

  std::list<std::unique_ptr<CJS_DelayData>> DelayDataToProcess;
  DelayDataToProcess.swap(delay_data_);
  for (const auto& pData : DelayDataToProcess) {
    CJS_Field::DoDelay(form_fill_env_.Get(), pData.get());
  }

  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_keywords(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "Keywords");
}

CJS_Result CJS_Document::set_keywords(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_mod_date(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "ModDate");
}

CJS_Result CJS_Document::set_mod_date(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_producer(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "Producer");
}

CJS_Result CJS_Document::set_producer(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_subject(CJS_Runtime* pRuntime) {
  return GetPropertyInternal(pRuntime, "Subject");
}

CJS_Result CJS_Document::set_subject(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_title(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }
  return GetPropertyInternal(pRuntime, "Title");
}

CJS_Result CJS_Document::set_title(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  // Read-only.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_num_pages(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }
  return CJS_Result::Success(
      pRuntime->NewNumber(form_fill_env_->GetPageCount()));
}

CJS_Result CJS_Document::set_num_pages(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::get_external(CJS_Runtime* pRuntime) {
  // In Chrome case, should always return true.
  return CJS_Result::Success(pRuntime->NewBoolean(true));
}

CJS_Result CJS_Document::set_external(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_filesize(CJS_Runtime* pRuntime) {
  return CJS_Result::Success(pRuntime->NewNumber(0));
}

CJS_Result CJS_Document::set_filesize(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::get_mouse_x(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_mouse_x(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_mouse_y(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_mouse_y(CJS_Runtime* pRuntime,
                                     v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_URL(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }
  return CJS_Result::Success(
      pRuntime->NewString(form_fill_env_->JS_docGetFilePath().AsStringView()));
}

CJS_Result CJS_Document::set_URL(CJS_Runtime* pRuntime,
                                 v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::get_base_URL(CJS_Runtime* pRuntime) {
  return CJS_Result::Success(pRuntime->NewString(base_url_.AsStringView()));
}

CJS_Result CJS_Document::set_base_URL(CJS_Runtime* pRuntime,
                                      v8::Local<v8::Value> vp) {
  base_url_ = pRuntime->ToWideString(vp);
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_calculate(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDFSDK_InteractiveForm* pInteractiveForm = GetSDKInteractiveForm();
  return CJS_Result::Success(
      pRuntime->NewBoolean(!!pInteractiveForm->IsCalculateEnabled()));
}

CJS_Result CJS_Document::set_calculate(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDFSDK_InteractiveForm* pInteractiveForm = GetSDKInteractiveForm();
  pInteractiveForm->EnableCalculate(pRuntime->ToBoolean(vp));
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_document_file_name(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  WideString wsFilePath = form_fill_env_->JS_docGetFilePath();
  size_t i = wsFilePath.GetLength();
  for (; i > 0; i--) {
    if (wsFilePath[i - 1] == L'\\' || wsFilePath[i - 1] == L'/') {
      break;
    }
  }
  if (i > 0 && i < wsFilePath.GetLength()) {
    return CJS_Result::Success(
        pRuntime->NewString(wsFilePath.AsStringView().Substr(i)));
  }
  return CJS_Result::Success(pRuntime->NewString(""));
}

CJS_Result CJS_Document::set_document_file_name(CJS_Runtime* pRuntime,
                                                v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::get_path(CJS_Runtime* pRuntime) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }
  return CJS_Result::Success(pRuntime->NewString(
      CJS_App::SysPathToPDFPath(form_fill_env_->JS_docGetFilePath())
          .AsStringView()));
}

CJS_Result CJS_Document::set_path(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::get_page_window_rect(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_page_window_rect(CJS_Runtime* pRuntime,
                                              v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_layout(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_layout(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::addLink(CJS_Runtime* pRuntime,
                                 pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::closeDoc(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getPageBox(CJS_Runtime* pRuntime,
                                    pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getAnnot(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  if (params.size() != 2) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  int nPageNo = pRuntime->ToInt32(params[0]);
  WideString swAnnotName = pRuntime->ToWideString(params[1]);
  CPDFSDK_PageView* pPageView = form_fill_env_->GetPageViewAtIndex(nPageNo);
  if (!pPageView) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDFSDK_AnnotIteration annot_iteration(pPageView);
  CPDFSDK_BAAnnot* pSDKBAAnnot = nullptr;
  for (const auto& pSDKAnnotCur : annot_iteration) {
    auto* pBAAnnot = pSDKAnnotCur->AsBAAnnot();
    if (pBAAnnot && pBAAnnot->GetAnnotName() == swAnnotName) {
      pSDKBAAnnot = pBAAnnot;
      break;
    }
  }
  if (!pSDKBAAnnot) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  v8::Local<v8::Object> pObj = pRuntime->NewFXJSBoundObject(
      CJS_Annot::GetObjDefnID(), FXJSOBJTYPE_DYNAMIC);
  if (pObj.IsEmpty()) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  auto* pJS_Annot = static_cast<CJS_Annot*>(
      CFXJS_Engine::GetBinding(pRuntime->GetIsolate(), pObj));
  if (!pJS_Annot) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  pJS_Annot->SetSDKAnnot(pSDKBAAnnot);
  return CJS_Result::Success(pJS_Annot->ToV8Object());
}

CJS_Result CJS_Document::getAnnots(CJS_Runtime* pRuntime,
                                   pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  // TODO(tonikitoo): Add support supported parameters as per
  // the PDF spec.

  int nPageNo = form_fill_env_->GetPageCount();
  v8::Local<v8::Array> annots = pRuntime->NewArray();
  for (int i = 0; i < nPageNo; ++i) {
    CPDFSDK_PageView* pPageView = form_fill_env_->GetPageViewAtIndex(i);
    if (!pPageView) {
      return CJS_Result::Failure(JSMessage::kBadObjectError);
    }

    CPDFSDK_AnnotIteration annot_iteration(pPageView);
    for (const auto& pSDKAnnotCur : annot_iteration) {
      if (!pSDKAnnotCur) {
        return CJS_Result::Failure(JSMessage::kBadObjectError);
      }

      v8::Local<v8::Object> pObj = pRuntime->NewFXJSBoundObject(
          CJS_Annot::GetObjDefnID(), FXJSOBJTYPE_DYNAMIC);
      if (pObj.IsEmpty()) {
        return CJS_Result::Failure(JSMessage::kBadObjectError);
      }

      auto* pJS_Annot = static_cast<CJS_Annot*>(
          CFXJS_Engine::GetBinding(pRuntime->GetIsolate(), pObj));
      pJS_Annot->SetSDKAnnot(pSDKAnnotCur->AsBAAnnot());
      pRuntime->PutArrayElement(
          annots, i,
          pJS_Annot ? v8::Local<v8::Value>(pJS_Annot->ToV8Object())
                    : v8::Local<v8::Value>());
    }
  }
  return CJS_Result::Success(annots);
}

CJS_Result CJS_Document::getAnnot3D(CJS_Runtime* pRuntime,
                                    pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success(pRuntime->NewUndefined());
}

CJS_Result CJS_Document::getAnnots3D(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getOCGs(CJS_Runtime* pRuntime,
                                 pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getLinks(CJS_Runtime* pRuntime,
                                  pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::addIcon(CJS_Runtime* pRuntime,
                                 pdfium::span<v8::Local<v8::Value>> params) {
  if (params.size() != 2) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }

  if (!params[1]->IsObject()) {
    return CJS_Result::Failure(JSMessage::kTypeError);
  }

  v8::Local<v8::Object> pObj = pRuntime->ToObject(params[1]);
  if (!JSGetObject<CJS_Icon>(pRuntime->GetIsolate(), pObj)) {
    return CJS_Result::Failure(JSMessage::kTypeError);
  }

  WideString swIconName = pRuntime->ToWideString(params[0]);
  icon_names_.push_back(swIconName);
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_icons(CJS_Runtime* pRuntime) {
  // TODO(tsepez): Maybe make consistent with Acrobat Reader behavior which
  // is to throw an exception under the default security settings.
  if (icon_names_.empty()) {
    return CJS_Result::Success(pRuntime->NewUndefined());
  }

  v8::Local<v8::Array> Icons = pRuntime->NewArray();
  int i = 0;
  for (const auto& name : icon_names_) {
    v8::Local<v8::Object> pObj = pRuntime->NewFXJSBoundObject(
        CJS_Icon::GetObjDefnID(), FXJSOBJTYPE_DYNAMIC);
    if (pObj.IsEmpty()) {
      return CJS_Result::Failure(JSMessage::kBadObjectError);
    }

    auto* pJS_Icon = static_cast<CJS_Icon*>(
        CFXJS_Engine::GetBinding(pRuntime->GetIsolate(), pObj));
    pJS_Icon->SetIconName(name);
    pRuntime->PutArrayElement(Icons, i++,
                              pJS_Icon
                                  ? v8::Local<v8::Value>(pJS_Icon->ToV8Object())
                                  : v8::Local<v8::Value>());
  }
  return CJS_Result::Success(Icons);
}

CJS_Result CJS_Document::set_icons(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  return CJS_Result::Failure(JSMessage::kReadOnlyError);
}

CJS_Result CJS_Document::getIcon(CJS_Runtime* pRuntime,
                                 pdfium::span<v8::Local<v8::Value>> params) {
  if (params.size() != 1) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }

  WideString swIconName = pRuntime->ToWideString(params[0]);
  auto it = std::ranges::find(icon_names_, swIconName);
  if (it == icon_names_.end()) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  v8::Local<v8::Object> pObj = pRuntime->NewFXJSBoundObject(
      CJS_Icon::GetObjDefnID(), FXJSOBJTYPE_DYNAMIC);
  if (pObj.IsEmpty()) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  auto* pJSIcon = static_cast<CJS_Icon*>(
      CFXJS_Engine::GetBinding(pRuntime->GetIsolate(), pObj));
  if (!pJSIcon) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  pJSIcon->SetIconName(*it);
  return CJS_Result::Success(pJSIcon->ToV8Object());
}

CJS_Result CJS_Document::removeIcon(CJS_Runtime* pRuntime,
                                    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, no supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::createDataObject(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not implemented.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_media(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_media(CJS_Runtime* pRuntime,
                                   v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::calculateNow(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  if (!form_fill_env_->HasPermissions(
          pdfium::access_permissions::kModifyContent |
          pdfium::access_permissions::kModifyAnnotation |
          pdfium::access_permissions::kFillForm)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  GetSDKInteractiveForm()->OnCalculate(nullptr);
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_collab(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_collab(CJS_Runtime* pRuntime,
                                    v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getPageNthWord(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  using pdfium::access_permissions::kExtractForAccessibility;
  if (!form_fill_env_->HasPermissions(kExtractForAccessibility)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  // TODO(tsepez): check maximum allowable params.

  int nPageNo = params.size() > 0 ? pRuntime->ToInt32(params[0]) : 0;
  int nWordNo = params.size() > 1 ? pRuntime->ToInt32(params[1]) : 0;
  bool bStrip = params.size() > 2 ? pRuntime->ToBoolean(params[2]) : true;

  CPDF_Document* document = form_fill_env_->GetPDFDocument();
  if (nPageNo < 0 || nPageNo >= document->GetPageCount()) {
    return CJS_Result::Failure(JSMessage::kValueError);
  }

  RetainPtr<CPDF_Dictionary> pPageDict =
      document->GetMutablePageDictionary(nPageNo);
  if (!pPageDict) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  auto page = pdfium::MakeRetain<CPDF_Page>(document, std::move(pPageDict));
  page->AddPageImageCache();
  page->ParseContent();

  int nWords = 0;
  WideString swRet;
  for (auto& pPageObj : *page) {
    if (pPageObj->IsActive() && pPageObj->IsText()) {
      CPDF_TextObject* pTextObj = pPageObj->AsText();
      int nObjWords = pTextObj->CountWords();
      if (nWords + nObjWords >= nWordNo) {
        swRet = pTextObj->GetWordString(nWordNo - nWords);
        break;
      }
      nWords += nObjWords;
    }
  }

  if (bStrip) {
    swRet.TrimWhitespace();
  }
  return CJS_Result::Success(pRuntime->NewString(swRet.AsStringView()));
}

CJS_Result CJS_Document::getPageNthWordQuads(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  using pdfium::access_permissions::kExtractForAccessibility;
  if (!form_fill_env_->HasPermissions(kExtractForAccessibility)) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  return CJS_Result::Failure(JSMessage::kNotSupportedError);
}

CJS_Result CJS_Document::getPageNumWords(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  using pdfium::access_permissions::kExtractForAccessibility;
  if (!form_fill_env_->HasPermissions(kExtractForAccessibility)) {
    return CJS_Result::Failure(JSMessage::kPermissionError);
  }

  int nPageNo = params.size() > 0 ? pRuntime->ToInt32(params[0]) : 0;
  CPDF_Document* document = form_fill_env_->GetPDFDocument();
  if (nPageNo < 0 || nPageNo >= document->GetPageCount()) {
    return CJS_Result::Failure(JSMessage::kValueError);
  }

  RetainPtr<CPDF_Dictionary> pPageDict =
      document->GetMutablePageDictionary(nPageNo);
  if (!pPageDict) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  auto page = pdfium::MakeRetain<CPDF_Page>(document, std::move(pPageDict));
  page->AddPageImageCache();
  page->ParseContent();

  int nWords = 0;
  for (auto& pPageObj : *page) {
    if (pPageObj->IsActive() && pPageObj->IsText()) {
      nWords += pPageObj->AsText()->CountWords();
    }
  }
  return CJS_Result::Success(pRuntime->NewNumber(nWords));
}

CJS_Result CJS_Document::getPrintParams(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  return CJS_Result::Failure(JSMessage::kNotSupportedError);
}

CJS_Result CJS_Document::get_zoom(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_zoom(CJS_Runtime* pRuntime,
                                  v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::get_zoom_type(CJS_Runtime* pRuntime) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::set_zoom_type(CJS_Runtime* pRuntime,
                                       v8::Local<v8::Value> vp) {
  return CJS_Result::Success();
}

CJS_Result CJS_Document::deletePages(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::extractPages(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::insertPages(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::replacePages(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::getURL(CJS_Runtime* pRuntime,
                                pdfium::span<v8::Local<v8::Value>> params) {
  // Unsafe, not supported.
  return CJS_Result::Success();
}

CJS_Result CJS_Document::gotoNamedDest(
    CJS_Runtime* pRuntime,
    pdfium::span<v8::Local<v8::Value>> params) {
  if (params.size() != 1) {
    return CJS_Result::Failure(JSMessage::kParamError);
  }

  if (!form_fill_env_) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDF_Document* document = form_fill_env_->GetPDFDocument();
  RetainPtr<const CPDF_Array> dest_array = CPDF_NameTree::LookupNamedDest(
      document, pRuntime->ToByteString(params[0]));
  if (!dest_array) {
    return CJS_Result::Failure(JSMessage::kBadObjectError);
  }

  CPDF_Dest dest(std::move(dest_array));
  std::vector<float> positions = dest.GetScrollPositionArray();
  pRuntime->BeginBlock();
  form_fill_env_->DoGoToAction(dest.GetDestPageIndex(document),
                               dest.GetZoomMode(), positions);
  pRuntime->EndBlock();
  return CJS_Result::Success();
}

void CJS_Document::AddDelayData(std::unique_ptr<CJS_DelayData> pData) {
  delay_data_.push_back(std::move(pData));
}

void CJS_Document::DoFieldDelay(const WideString& sFieldName,
                                int nControlIndex) {
  std::vector<std::unique_ptr<CJS_DelayData>> delayed_data;
  auto iter = delay_data_.begin();
  while (iter != delay_data_.end()) {
    auto old = iter++;
    if ((*old)->sFieldName == sFieldName &&
        (*old)->nControlIndex == nControlIndex) {
      delayed_data.push_back(std::move(*old));
      delay_data_.erase(old);
    }
  }

  for (const auto& pData : delayed_data) {
    CJS_Field::DoDelay(form_fill_env_.Get(), pData.get());
  }
}

CPDF_InteractiveForm* CJS_Document::GetCoreInteractiveForm() {
  return GetSDKInteractiveForm()->GetInteractiveForm();
}

CPDFSDK_InteractiveForm* CJS_Document::GetSDKInteractiveForm() {
  return form_fill_env_->GetInteractiveForm();
}
