// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fxfa/parser/cxfa_rectangle.h"

#include <math.h>

#include <utility>

#include "core/fxcrt/check.h"
#include "core/fxcrt/notreached.h"
#include "fxjs/xfa/cjx_node.h"
#include "xfa/fgas/graphics/cfgas_gegraphics.h"
#include "xfa/fxfa/parser/cxfa_corner.h"
#include "xfa/fxfa/parser/cxfa_document.h"
#include "xfa/fxfa/parser/cxfa_stroke.h"

namespace {

const CXFA_Node::PropertyData kRectanglePropertyData[] = {
    {XFA_Element::Edge, 4, {}},
    {XFA_Element::Corner, 4, {}},
    {XFA_Element::Fill, 1, {}},
};

const CXFA_Node::AttributeData kRectangleAttributeData[] = {
    {XFA_Attribute::Id, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Use, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Usehref, XFA_AttributeType::CData, nullptr},
    {XFA_Attribute::Hand, XFA_AttributeType::Enum,
     (void*)XFA_AttributeValue::Even},
};

}  // namespace

// static
CXFA_Rectangle* CXFA_Rectangle::FromNode(CXFA_Node* pNode) {
  return pNode && pNode->GetElementType() == XFA_Element::Rectangle
             ? static_cast<CXFA_Rectangle*>(pNode)
             : nullptr;
}

CXFA_Rectangle::CXFA_Rectangle(CXFA_Document* doc, XFA_PacketType packet)
    : CXFA_Box(doc,
               packet,
               {XFA_XDPPACKET::kTemplate, XFA_XDPPACKET::kForm},
               XFA_ObjectType::Node,
               XFA_Element::Rectangle,
               kRectanglePropertyData,
               kRectangleAttributeData,
               cppgc::MakeGarbageCollected<CJX_Node>(
                   doc->GetHeap()->GetAllocationHandle(),
                   this)) {}

CXFA_Rectangle::CXFA_Rectangle(CXFA_Document* doc,
                               XFA_PacketType ePacket,
                               Mask<XFA_XDPPACKET> validPackets,
                               XFA_ObjectType oType,
                               XFA_Element eType,
                               pdfium::span<const PropertyData> properties,
                               pdfium::span<const AttributeData> attributes,
                               CJX_Object* js_node)
    : CXFA_Box(doc,
               ePacket,
               validPackets,
               oType,
               eType,
               properties,
               attributes,
               js_node) {}

CXFA_Rectangle::~CXFA_Rectangle() = default;

void CXFA_Rectangle::GetFillPath(const std::vector<CXFA_Stroke*>& strokes,
                                 const CFX_RectF& rtWidget,
                                 CFGAS_GEPath* fillPath) {
  bool bSameStyles = true;
  CXFA_Stroke* stroke1 = strokes[0];
  for (int32_t i = 1; i < 8; i++) {
    CXFA_Stroke* stroke2 = strokes[i];
    if (!stroke1->SameStyles(stroke2, {})) {
      bSameStyles = false;
      break;
    }
    stroke1 = stroke2;
  }

  if (bSameStyles) {
    stroke1 = strokes[0];
    for (int32_t i = 2; i < 8; i += 2) {
      CXFA_Stroke* stroke2 = strokes[i];
      if (!stroke1->SameStyles(stroke2,
                               {CXFA_Stroke::SameStyleOption::kNoPresence,
                                CXFA_Stroke::SameStyleOption::kCorner})) {
        bSameStyles = false;
        break;
      }
      stroke1 = stroke2;
    }
    if (bSameStyles) {
      stroke1 = strokes[0];
      if (stroke1->IsInverted()) {
        bSameStyles = false;
      }
      if (stroke1->GetJoinType() != XFA_AttributeValue::Square) {
        bSameStyles = false;
      }
    }
  }
  if (bSameStyles) {
    fillPath->AddRectangle(rtWidget.left, rtWidget.top, rtWidget.width,
                           rtWidget.height);
    return;
  }

  for (int32_t i = 0; i < 8; i += 2) {
    float sx = 0.0f;
    float sy = 0.0f;
    float vx = 1.0f;
    float vy = 1.0f;
    float nx = 1.0f;
    float ny = 1.0f;
    CFX_PointF cp1;
    CFX_PointF cp2;
    CXFA_Stroke* corner1 = strokes[i];
    CXFA_Stroke* corner2 = strokes[(i + 2) % 8];
    float fRadius1 = corner1->GetRadius();
    float fRadius2 = corner2->GetRadius();
    bool bInverted = corner1->IsInverted();
    bool bRound = corner1->GetJoinType() == XFA_AttributeValue::Round;
    if (bRound) {
      sy = FXSYS_PI / 2;
    }
    switch (i) {
      case 0:
        cp1 = rtWidget.TopLeft();
        cp2 = rtWidget.TopRight();
        vx = 1;
        vy = 1;
        nx = -1;
        ny = 0;
        if (bRound) {
          sx = bInverted ? FXSYS_PI / 2 : FXSYS_PI;
        } else {
          sx = 1;
          sy = 0;
        }
        break;
      case 2:
        cp1 = rtWidget.TopRight();
        cp2 = rtWidget.BottomRight();
        vx = -1;
        vy = 1;
        nx = 0;
        ny = -1;
        if (bRound) {
          sx = bInverted ? FXSYS_PI : FXSYS_PI * 3 / 2;
        } else {
          sx = 0;
          sy = 1;
        }
        break;
      case 4:
        cp1 = rtWidget.BottomRight();
        cp2 = rtWidget.BottomLeft();
        vx = -1;
        vy = -1;
        nx = 1;
        ny = 0;
        if (bRound) {
          sx = bInverted ? FXSYS_PI * 3 / 2 : 0;
        } else {
          sx = -1;
          sy = 0;
        }
        break;
      case 6:
        cp1 = rtWidget.BottomLeft();
        cp2 = rtWidget.TopLeft();
        vx = 1;
        vy = -1;
        nx = 0;
        ny = 1;
        if (bRound) {
          sx = bInverted ? 0 : FXSYS_PI / 2;
        } else {
          sx = 0;
          sy = -1;
        }
        break;
    }
    if (i == 0) {
      fillPath->MoveTo(CFX_PointF(cp1.x, cp1.y + fRadius1));
    }

    if (bRound) {
      if (fRadius1 < 0) {
        sx -= FXSYS_PI;
      }
      if (bInverted) {
        sy *= -1;
      }

      CFX_RectF rtRadius(cp1.x, cp1.y, fRadius1 * 2 * vx, fRadius1 * 2 * vy);
      rtRadius.Normalize();
      if (bInverted) {
        rtRadius.Offset(-fRadius1 * vx, -fRadius1 * vy);
      }

      fillPath->ArcTo(rtRadius.TopLeft(), rtRadius.Size(), sx, sy);
    } else {
      CFX_PointF cp;
      if (bInverted) {
        cp.x = cp1.x + fRadius1 * vx;
        cp.y = cp1.y + fRadius1 * vy;
      } else {
        cp = cp1;
      }
      fillPath->LineTo(cp);
      fillPath->LineTo(
          CFX_PointF(cp1.x + fRadius1 * sx, cp1.y + fRadius1 * sy));
    }
    fillPath->LineTo(CFX_PointF(cp2.x + fRadius2 * nx, cp2.y + fRadius2 * ny));
  }
}

void CXFA_Rectangle::Draw(const std::vector<CXFA_Stroke*>& strokes,
                          CFGAS_GEGraphics* pGS,
                          CFX_RectF rtWidget,
                          const CFX_Matrix& matrix) {
  bool bVisible = false;
  for (int32_t j = 0; j < 4; j++) {
    if (strokes[j * 2 + 1]->IsVisible()) {
      bVisible = true;
      break;
    }
  }
  if (!bVisible) {
    return;
  }

  for (int32_t i = 1; i < 8; i += 2) {
    float fThickness = fmax(0.0, strokes[i]->GetThickness());
    float fHalf = fThickness / 2;
    XFA_AttributeValue iHand = GetHand();
    switch (i) {
      case 1:
        if (iHand == XFA_AttributeValue::Left) {
          rtWidget.top -= fHalf;
          rtWidget.height += fHalf;
        } else if (iHand == XFA_AttributeValue::Right) {
          rtWidget.top += fHalf;
          rtWidget.height -= fHalf;
        }
        break;
      case 3:
        if (iHand == XFA_AttributeValue::Left) {
          rtWidget.width += fHalf;
        } else if (iHand == XFA_AttributeValue::Right) {
          rtWidget.width -= fHalf;
        }
        break;
      case 5:
        if (iHand == XFA_AttributeValue::Left) {
          rtWidget.height += fHalf;
        } else if (iHand == XFA_AttributeValue::Right) {
          rtWidget.height -= fHalf;
        }
        break;
      case 7:
        if (iHand == XFA_AttributeValue::Left) {
          rtWidget.left -= fHalf;
          rtWidget.width += fHalf;
        } else if (iHand == XFA_AttributeValue::Right) {
          rtWidget.left += fHalf;
          rtWidget.width -= fHalf;
        }
        break;
    }
  }
  Stroke(strokes, pGS, rtWidget, matrix);
}

void CXFA_Rectangle::Stroke(const std::vector<CXFA_Stroke*>& strokes,
                            CFGAS_GEGraphics* pGS,
                            CFX_RectF rtWidget,
                            const CFX_Matrix& matrix) {
  auto [i3DType, bVisible, fThickness] = Get3DStyle();
  if (i3DType != XFA_AttributeValue::Unknown) {
    if (!bVisible || fThickness < 0.001f) {
      return;
    }

    switch (i3DType) {
      case XFA_AttributeValue::Lowered:
        StrokeLowered(pGS, rtWidget, fThickness, matrix);
        break;
      case XFA_AttributeValue::Raised:
        StrokeRaised(pGS, rtWidget, fThickness, matrix);
        break;
      case XFA_AttributeValue::Etched:
        StrokeEtched(pGS, rtWidget, fThickness, matrix);
        break;
      case XFA_AttributeValue::Embossed:
        StrokeEmbossed(pGS, rtWidget, fThickness, matrix);
        break;
      default:
        NOTREACHED();
    }
    return;
  }

  bool bClose = false;
  bool bSameStyles = true;
  CXFA_Stroke* stroke1 = strokes[0];
  for (int32_t i = 1; i < 8; i++) {
    CXFA_Stroke* stroke2 = strokes[i];
    if (!stroke1->SameStyles(stroke2, {})) {
      bSameStyles = false;
      break;
    }
    stroke1 = stroke2;
  }
  if (bSameStyles) {
    stroke1 = strokes[0];
    bClose = true;
    for (int32_t i = 2; i < 8; i += 2) {
      CXFA_Stroke* stroke2 = strokes[i];
      if (!stroke1->SameStyles(stroke2,
                               {CXFA_Stroke::SameStyleOption::kNoPresence,
                                CXFA_Stroke::SameStyleOption::kCorner})) {
        bSameStyles = false;
        break;
      }
      stroke1 = stroke2;
    }
    if (bSameStyles) {
      stroke1 = strokes[0];
      if (stroke1->IsInverted()) {
        bSameStyles = false;
      }
      if (stroke1->GetJoinType() != XFA_AttributeValue::Square) {
        bSameStyles = false;
      }
    }
  }

  bool bStart = true;
  CFGAS_GEPath path;
  for (int32_t i = 0; i < 8; i++) {
    CXFA_Stroke* stroke = strokes[i];
    if ((i % 1) == 0 && stroke->GetRadius() < 0) {
      bool bEmpty = path.IsEmpty();
      if (!bEmpty) {
        if (stroke) {
          stroke->Stroke(pGS, path, matrix);
        }
        path.Clear();
      }
      bStart = true;
      continue;
    }
    GetPath(strokes, rtWidget, path, i, bStart, !bSameStyles);

    bStart = !stroke->SameStyles(strokes[(i + 1) % 8], {});
    if (bStart) {
      if (stroke) {
        stroke->Stroke(pGS, path, matrix);
      }
      path.Clear();
    }
  }
  bool bEmpty = path.IsEmpty();
  if (!bEmpty) {
    if (bClose) {
      path.Close();
    }
    if (strokes[7]) {
      strokes[7]->Stroke(pGS, path, matrix);
    }
  }
}

void CXFA_Rectangle::StrokeRect(CFGAS_GEGraphics* pGraphic,
                                const CFX_RectF& rt,
                                float fLineWidth,
                                const CFX_Matrix& matrix,
                                FX_ARGB argbTopLeft,
                                FX_ARGB argbBottomRight) {
  float fBottom = rt.bottom();
  float fRight = rt.right();
  CFGAS_GEPath pathLT;
  pathLT.MoveTo(CFX_PointF(rt.left, fBottom));
  pathLT.LineTo(CFX_PointF(rt.left, rt.top));
  pathLT.LineTo(CFX_PointF(fRight, rt.top));
  pathLT.LineTo(CFX_PointF(fRight - fLineWidth, rt.top + fLineWidth));
  pathLT.LineTo(CFX_PointF(rt.left + fLineWidth, rt.top + fLineWidth));
  pathLT.LineTo(CFX_PointF(rt.left + fLineWidth, fBottom - fLineWidth));
  pathLT.LineTo(CFX_PointF(rt.left, fBottom));
  pGraphic->SetFillColor(CFGAS_GEColor(argbTopLeft));
  pGraphic->FillPath(pathLT, CFX_FillRenderOptions::FillType::kWinding, matrix);

  CFGAS_GEPath pathRB;
  pathRB.MoveTo(CFX_PointF(fRight, rt.top));
  pathRB.LineTo(CFX_PointF(fRight, fBottom));
  pathRB.LineTo(CFX_PointF(rt.left, fBottom));
  pathRB.LineTo(CFX_PointF(rt.left + fLineWidth, fBottom - fLineWidth));
  pathRB.LineTo(CFX_PointF(fRight - fLineWidth, fBottom - fLineWidth));
  pathRB.LineTo(CFX_PointF(fRight - fLineWidth, rt.top + fLineWidth));
  pathRB.LineTo(CFX_PointF(fRight, rt.top));
  pGraphic->SetFillColor(CFGAS_GEColor(argbBottomRight));
  pGraphic->FillPath(pathRB, CFX_FillRenderOptions::FillType::kWinding, matrix);
}

void CXFA_Rectangle::StrokeLowered(CFGAS_GEGraphics* pGS,
                                   CFX_RectF rt,
                                   float fThickness,
                                   const CFX_Matrix& matrix) {
  float fHalfWidth = fThickness / 2.0f;
  CFX_RectF rtInner(rt);
  rtInner.Deflate(fHalfWidth, fHalfWidth);

  CFGAS_GEPath path;
  path.AddRectangle(rt.left, rt.top, rt.width, rt.height);
  path.AddRectangle(rtInner.left, rtInner.top, rtInner.width, rtInner.height);
  pGS->SetFillColor(CFGAS_GEColor(0xFF000000));
  pGS->FillPath(path, CFX_FillRenderOptions::FillType::kEvenOdd, matrix);
  StrokeRect(pGS, rtInner, fHalfWidth, matrix, 0xFF808080, 0xFFC0C0C0);
}

void CXFA_Rectangle::StrokeRaised(CFGAS_GEGraphics* pGS,
                                  CFX_RectF rt,
                                  float fThickness,
                                  const CFX_Matrix& matrix) {
  float fHalfWidth = fThickness / 2.0f;
  CFX_RectF rtInner(rt);
  rtInner.Deflate(fHalfWidth, fHalfWidth);

  CFGAS_GEPath path;
  path.AddRectangle(rt.left, rt.top, rt.width, rt.height);
  path.AddRectangle(rtInner.left, rtInner.top, rtInner.width, rtInner.height);
  pGS->SetFillColor(CFGAS_GEColor(0xFF000000));
  pGS->FillPath(path, CFX_FillRenderOptions::FillType::kEvenOdd, matrix);
  StrokeRect(pGS, rtInner, fHalfWidth, matrix, 0xFFFFFFFF, 0xFF808080);
}

void CXFA_Rectangle::StrokeEtched(CFGAS_GEGraphics* pGS,
                                  CFX_RectF rt,
                                  float fThickness,
                                  const CFX_Matrix& matrix) {
  float fHalfWidth = fThickness / 2.0f;
  StrokeRect(pGS, rt, fThickness, matrix, 0xFF808080, 0xFFFFFFFF);

  CFX_RectF rtInner(rt);
  rtInner.Deflate(fHalfWidth, fHalfWidth);
  StrokeRect(pGS, rtInner, fHalfWidth, matrix, 0xFFFFFFFF, 0xFF808080);
}

void CXFA_Rectangle::StrokeEmbossed(CFGAS_GEGraphics* pGS,
                                    CFX_RectF rt,
                                    float fThickness,
                                    const CFX_Matrix& matrix) {
  float fHalfWidth = fThickness / 2.0f;
  StrokeRect(pGS, rt, fThickness, matrix, 0xFF808080, 0xFF000000);

  CFX_RectF rtInner(rt);
  rtInner.Deflate(fHalfWidth, fHalfWidth);
  StrokeRect(pGS, rtInner, fHalfWidth, matrix, 0xFF000000, 0xFF808080);
}

void CXFA_Rectangle::GetPath(const std::vector<CXFA_Stroke*>& strokes,
                             CFX_RectF rtWidget,
                             CFGAS_GEPath& path,
                             int32_t nIndex,
                             bool bStart,
                             bool bCorner) {
  DCHECK(nIndex >= 0);
  DCHECK(nIndex < 8);

  int32_t n = (nIndex & 1) ? nIndex - 1 : nIndex;
  CXFA_Stroke* corner1 = strokes[n];
  CXFA_Stroke* corner2 = strokes[(n + 2) % 8];
  float fRadius1 = bCorner ? corner1->GetRadius() : 0.0f;
  float fRadius2 = bCorner ? corner2->GetRadius() : 0.0f;
  bool bInverted = corner1->IsInverted();
  float offsetY = 0.0f;
  float offsetX = 0.0f;
  bool bRound = corner1->GetJoinType() == XFA_AttributeValue::Round;
  float halfAfter = 0.0f;
  float halfBefore = 0.0f;

  CXFA_Stroke* stroke = strokes[nIndex];
  if (stroke->IsCorner()) {
    CXFA_Stroke* strokeBefore = strokes[(nIndex + 1 * 8 - 1) % 8];
    CXFA_Stroke* strokeAfter = strokes[nIndex + 1];
    if (stroke->IsInverted()) {
      if (!stroke->SameStyles(strokeBefore, {})) {
        halfBefore = strokeBefore->GetThickness() / 2;
      }
      if (!stroke->SameStyles(strokeAfter, {})) {
        halfAfter = strokeAfter->GetThickness() / 2;
      }
    }
  } else {
    CXFA_Stroke* strokeBefore = strokes[(nIndex + 8 - 2) % 8];
    CXFA_Stroke* strokeAfter = strokes[(nIndex + 2) % 8];
    if (!bRound && !bInverted) {
      halfBefore = strokeBefore->GetThickness() / 2;
      halfAfter = strokeAfter->GetThickness() / 2;
    }
  }

  float offsetEX = 0.0f;
  float offsetEY = 0.0f;
  float sx = 0.0f;
  float sy = 0.0f;
  float vx = 1.0f;
  float vy = 1.0f;
  float nx = 1.0f;
  float ny = 1.0f;
  CFX_PointF cpStart;
  CFX_PointF cp1;
  CFX_PointF cp2;
  if (bRound) {
    sy = FXSYS_PI / 2;
  }

  switch (nIndex) {
    case 0:
    case 1:
      cp1 = rtWidget.TopLeft();
      cp2 = rtWidget.TopRight();
      if (nIndex == 0) {
        cpStart.x = cp1.x - halfBefore;
        cpStart.y = cp1.y + fRadius1;
        offsetY = -halfAfter;
      } else {
        cpStart.x = cp1.x + fRadius1 - halfBefore;
        cpStart.y = cp1.y;
        offsetEX = halfAfter;
      }
      vx = 1;
      vy = 1;
      nx = -1;
      ny = 0;
      if (bRound) {
        sx = bInverted ? FXSYS_PI / 2 : FXSYS_PI;
      } else {
        sx = 1;
        sy = 0;
      }
      break;
    case 2:
    case 3:
      cp1 = rtWidget.TopRight();
      cp2 = rtWidget.BottomRight();
      if (nIndex == 2) {
        cpStart.x = cp1.x - fRadius1;
        cpStart.y = cp1.y - halfBefore;
        offsetX = halfAfter;
      } else {
        cpStart.x = cp1.x;
        cpStart.y = cp1.y + fRadius1 - halfBefore;
        offsetEY = halfAfter;
      }
      vx = -1;
      vy = 1;
      nx = 0;
      ny = -1;
      if (bRound) {
        sx = bInverted ? FXSYS_PI : FXSYS_PI * 3 / 2;
      } else {
        sx = 0;
        sy = 1;
      }
      break;
    case 4:
    case 5:
      cp1 = rtWidget.BottomRight();
      cp2 = rtWidget.BottomLeft();
      if (nIndex == 4) {
        cpStart.x = cp1.x + halfBefore;
        cpStart.y = cp1.y - fRadius1;
        offsetY = halfAfter;
      } else {
        cpStart.x = cp1.x - fRadius1 + halfBefore;
        cpStart.y = cp1.y;
        offsetEX = -halfAfter;
      }
      vx = -1;
      vy = -1;
      nx = 1;
      ny = 0;
      if (bRound) {
        sx = bInverted ? FXSYS_PI * 3 / 2 : 0;
      } else {
        sx = -1;
        sy = 0;
      }
      break;
    case 6:
    case 7:
      cp1 = rtWidget.BottomLeft();
      cp2 = rtWidget.TopLeft();
      if (nIndex == 6) {
        cpStart.x = cp1.x + fRadius1;
        cpStart.y = cp1.y + halfBefore;
        offsetX = -halfAfter;
      } else {
        cpStart.x = cp1.x;
        cpStart.y = cp1.y - fRadius1 + halfBefore;
        offsetEY = -halfAfter;
      }
      vx = 1;
      vy = -1;
      nx = 0;
      ny = 1;
      if (bRound) {
        sx = bInverted ? 0 : FXSYS_PI / 2;
      } else {
        sx = 0;
        sy = -1;
      }
      break;
  }
  if (bStart) {
    path.MoveTo(cpStart);
  }
  if (nIndex & 1) {
    path.LineTo(CFX_PointF(cp2.x + fRadius2 * nx + offsetEX,
                           cp2.y + fRadius2 * ny + offsetEY));
    return;
  }
  if (bRound) {
    if (fRadius1 < 0) {
      sx -= FXSYS_PI;
    }
    if (bInverted) {
      sy *= -1;
    }

    CFX_RectF rtRadius(cp1.x + offsetX * 2, cp1.y + offsetY * 2,
                       fRadius1 * 2 * vx - offsetX * 2,
                       fRadius1 * 2 * vy - offsetY * 2);
    rtRadius.Normalize();
    if (bInverted) {
      rtRadius.Offset(-fRadius1 * vx, -fRadius1 * vy);
    }

    path.ArcTo(rtRadius.TopLeft(), rtRadius.Size(), sx, sy);
  } else {
    CFX_PointF cp;
    if (bInverted) {
      cp.x = cp1.x + fRadius1 * vx;
      cp.y = cp1.y + fRadius1 * vy;
    } else {
      cp = cp1;
    }
    path.LineTo(cp);
    path.LineTo(CFX_PointF(cp1.x + fRadius1 * sx + offsetX,
                           cp1.y + fRadius1 * sy + offsetY));
  }
}
