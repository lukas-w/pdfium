// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/cxfa_textprovider.h"

#include "core/fxcrt/check.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmlnode.h"
#include "fxjs/xfa/cfxjse_engine.h"
#include "fxjs/xfa/cfxjse_value.h"
#include "fxjs/xfa/cjx_object.h"
#include "xfa/fde/cfde_textout.h"
#include "xfa/fxfa/cxfa_eventparam.h"
#include "xfa/fxfa/cxfa_ffapp.h"
#include "xfa/fxfa/cxfa_ffcheckbutton.h"
#include "xfa/fxfa/cxfa_ffdoc.h"
#include "xfa/fxfa/cxfa_ffdocview.h"
#include "xfa/fxfa/cxfa_fffield.h"
#include "xfa/fxfa/cxfa_ffpageview.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "xfa/fxfa/cxfa_fontmgr.h"
#include "xfa/fxfa/cxfa_fwladapterwidgetmgr.h"
#include "xfa/fxfa/parser/cxfa_caption.h"
#include "xfa/fxfa/parser/cxfa_font.h"
#include "xfa/fxfa/parser/cxfa_items.h"
#include "xfa/fxfa/parser/cxfa_localevalue.h"
#include "xfa/fxfa/parser/cxfa_node.h"
#include "xfa/fxfa/parser/cxfa_para.h"
#include "xfa/fxfa/parser/cxfa_value.h"
#include "xfa/fxfa/parser/xfa_utils.h"

CXFA_TextProvider::CXFA_TextProvider(CXFA_Node* pNode, Type eType)
    : node_(pNode), type_(eType) {
  DCHECK(node_);
}

CXFA_TextProvider::~CXFA_TextProvider() = default;

void CXFA_TextProvider::Trace(cppgc::Visitor* visitor) const {
  visitor->Trace(node_);
}

CXFA_Node* CXFA_TextProvider::GetTextNode(bool* bRichText) {
  *bRichText = false;
  if (type_ == Type::kText) {
    CXFA_Value* pValueNode =
        node_->GetChild<CXFA_Value>(0, XFA_Element::Value, false);
    if (!pValueNode) {
      return nullptr;
    }

    CXFA_Node* pChildNode = pValueNode->GetFirstChild();
    if (pChildNode && pChildNode->GetElementType() == XFA_Element::ExData) {
      std::optional<WideString> contentType =
          pChildNode->JSObject()->TryAttribute(XFA_Attribute::ContentType,
                                               false);
      if (contentType.has_value() &&
          contentType.value().EqualsASCII("text/html")) {
        *bRichText = true;
      }
    }
    return pChildNode;
  }

  if (type_ == Type::kCaption) {
    CXFA_Caption* pCaptionNode =
        node_->GetChild<CXFA_Caption>(0, XFA_Element::Caption, false);
    if (!pCaptionNode) {
      return nullptr;
    }

    CXFA_Value* pValueNode =
        pCaptionNode->GetChild<CXFA_Value>(0, XFA_Element::Value, false);
    if (!pValueNode) {
      return nullptr;
    }

    CXFA_Node* pChildNode = pValueNode->GetFirstChild();
    if (pChildNode && pChildNode->GetElementType() == XFA_Element::ExData) {
      std::optional<WideString> contentType =
          pChildNode->JSObject()->TryAttribute(XFA_Attribute::ContentType,
                                               false);
      if (contentType.has_value() &&
          contentType.value().EqualsASCII("text/html")) {
        *bRichText = true;
      }
    }
    return pChildNode;
  }

  CXFA_Items* pItemNode =
      node_->GetChild<CXFA_Items>(0, XFA_Element::Items, false);
  if (!pItemNode) {
    return nullptr;
  }

  CXFA_Node* pNode = pItemNode->GetFirstChild();
  while (pNode) {
    WideString wsName = pNode->JSObject()->GetCData(XFA_Attribute::Name);
    if (type_ == Type::kRollover && wsName.EqualsASCII("rollover")) {
      return pNode;
    }
    if (type_ == Type::kDown && wsName.EqualsASCII("down")) {
      return pNode;
    }

    pNode = pNode->GetNextSibling();
  }
  return nullptr;
}

CXFA_Para* CXFA_TextProvider::GetParaIfExists() {
  if (type_ == Type::kText) {
    return node_->GetParaIfExists();
  }

  CXFA_Caption* pNode =
      node_->GetChild<CXFA_Caption>(0, XFA_Element::Caption, false);
  return pNode->GetChild<CXFA_Para>(0, XFA_Element::Para, false);
}

CXFA_Font* CXFA_TextProvider::GetFontIfExists() {
  if (type_ == Type::kText) {
    return node_->GetFontIfExists();
  }

  CXFA_Caption* pNode =
      node_->GetChild<CXFA_Caption>(0, XFA_Element::Caption, false);
  CXFA_Font* font = pNode->GetChild<CXFA_Font>(0, XFA_Element::Font, false);
  return font ? font : node_->GetFontIfExists();
}

bool CXFA_TextProvider::IsCheckButtonAndAutoWidth() const {
  if (node_->GetFFWidgetType() != XFA_FFWidgetType::kCheckButton) {
    return false;
  }
  return !node_->TryWidth().has_value();
}

std::optional<WideString> CXFA_TextProvider::GetEmbeddedObj(
    const WideString& wsAttr) const {
  if (type_ != Type::kText) {
    return std::nullopt;
  }

  CXFA_Node* pParent = node_->GetParent();
  CXFA_Document* document = node_->GetDocument();
  CXFA_Node* pIDNode = nullptr;
  if (pParent) {
    pIDNode = document->GetNodeByID(pParent, wsAttr.AsStringView());
  }

  if (!pIDNode) {
    pIDNode =
        document->GetNodeByID(ToNode(document->GetXFAObject(XFA_HASHCODE_Form)),
                              wsAttr.AsStringView());
  }
  if (!pIDNode || !pIDNode->IsWidgetReady()) {
    return std::nullopt;
  }

  return pIDNode->GetValue(XFA_ValuePicture::kDisplay);
}
