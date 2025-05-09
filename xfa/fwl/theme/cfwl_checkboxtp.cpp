// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "xfa/fwl/theme/cfwl_checkboxtp.h"

#include <math.h>

#include <array>
#include <iterator>

#include "core/fxge/cfx_path.h"
#include "xfa/fde/cfde_textout.h"
#include "xfa/fgas/graphics/cfgas_gecolor.h"
#include "xfa/fgas/graphics/cfgas_gegraphics.h"
#include "xfa/fgas/graphics/cfgas_gepath.h"
#include "xfa/fwl/cfwl_checkbox.h"
#include "xfa/fwl/cfwl_themebackground.h"
#include "xfa/fwl/cfwl_themetext.h"
#include "xfa/fwl/cfwl_widget.h"

namespace pdfium {

namespace {

constexpr int kSignPath = 100;

CFX_PointF ScaleBezierPoint(const CFX_PointF& point) {
  CFX_PointF scaled_point(point);
  scaled_point.x *= FXSYS_BEZIER;
  scaled_point.y *= FXSYS_BEZIER;
  return scaled_point;
}

}  // namespace

CFWL_CheckBoxTP::CFWL_CheckBoxTP() = default;

CFWL_CheckBoxTP::~CFWL_CheckBoxTP() = default;

void CFWL_CheckBoxTP::DrawText(const CFWL_ThemeText& pParams) {
  EnsureTTOInitialized(pParams.GetWidget()->GetThemeProvider());
  text_out_->SetTextColor(pParams.states_ & CFWL_PartState::kDisabled
                              ? FWLTHEME_CAPACITY_TextDisColor
                              : FWLTHEME_CAPACITY_TextColor);
  CFWL_WidgetTP::DrawText(pParams);
}

void CFWL_CheckBoxTP::DrawSignCheck(CFGAS_GEGraphics* pGraphics,
                                    const CFX_RectF& rtSign,
                                    FX_ARGB argbFill,
                                    const CFX_Matrix& matrix) {
  EnsureCheckPathInitialized(rtSign.width);
  DCHECK(check_path_);

  CFX_Matrix mt;
  mt.Translate(rtSign.left, rtSign.top);
  mt.Concat(matrix);

  CFGAS_GEGraphics::StateRestorer restorer(pGraphics);
  pGraphics->SetFillColor(CFGAS_GEColor(argbFill));
  pGraphics->FillPath(*check_path_, CFX_FillRenderOptions::FillType::kWinding,
                      mt);
}

void CFWL_CheckBoxTP::DrawSignCircle(CFGAS_GEGraphics* pGraphics,
                                     const CFX_RectF& rtSign,
                                     FX_ARGB argbFill,
                                     const CFX_Matrix& matrix) {
  CFGAS_GEPath path;
  path.AddEllipse(rtSign);

  CFGAS_GEGraphics::StateRestorer restorer(pGraphics);
  pGraphics->SetFillColor(CFGAS_GEColor(argbFill));
  pGraphics->FillPath(path, CFX_FillRenderOptions::FillType::kWinding, matrix);
}

void CFWL_CheckBoxTP::DrawSignCross(CFGAS_GEGraphics* pGraphics,
                                    const CFX_RectF& rtSign,
                                    FX_ARGB argbFill,
                                    const CFX_Matrix& matrix) {
  CFGAS_GEPath path;
  const float fRight = rtSign.right();
  const float fBottom = rtSign.bottom();
  path.AddLine(rtSign.TopLeft(), CFX_PointF(fRight, fBottom));
  path.AddLine(CFX_PointF(rtSign.left, fBottom),
               CFX_PointF(fRight, rtSign.top));

  CFGAS_GEGraphics::StateRestorer restorer(pGraphics);
  pGraphics->SetStrokeColor(CFGAS_GEColor(argbFill));
  pGraphics->SetLineWidth(1.0f);
  pGraphics->StrokePath(path, matrix);
}

void CFWL_CheckBoxTP::DrawSignDiamond(CFGAS_GEGraphics* pGraphics,
                                      const CFX_RectF& rtSign,
                                      FX_ARGB argbFill,
                                      const CFX_Matrix& matrix) {
  CFGAS_GEPath path;
  const float fWidth = rtSign.width;
  const float fHeight = rtSign.height;
  const float fBottom = rtSign.bottom();
  path.MoveTo(CFX_PointF(rtSign.left + fWidth / 2, rtSign.top));
  path.LineTo(CFX_PointF(rtSign.left, rtSign.top + fHeight / 2));
  path.LineTo(CFX_PointF(rtSign.left + fWidth / 2, fBottom));
  path.LineTo(CFX_PointF(rtSign.right(), rtSign.top + fHeight / 2));
  path.LineTo(CFX_PointF(rtSign.left + fWidth / 2, rtSign.top));

  CFGAS_GEGraphics::StateRestorer restorer(pGraphics);
  pGraphics->SetFillColor(CFGAS_GEColor(argbFill));
  pGraphics->FillPath(path, CFX_FillRenderOptions::FillType::kWinding, matrix);
}

void CFWL_CheckBoxTP::DrawSignSquare(CFGAS_GEGraphics* pGraphics,
                                     const CFX_RectF& rtSign,
                                     FX_ARGB argbFill,
                                     const CFX_Matrix& matrix) {
  CFGAS_GEPath path;
  path.AddRectangle(rtSign.left, rtSign.top, rtSign.width, rtSign.height);

  CFGAS_GEGraphics::StateRestorer restorer(pGraphics);
  pGraphics->SetFillColor(CFGAS_GEColor(argbFill));
  pGraphics->FillPath(path, CFX_FillRenderOptions::FillType::kWinding, matrix);
}

void CFWL_CheckBoxTP::DrawSignStar(CFGAS_GEGraphics* pGraphics,
                                   const CFX_RectF& rtSign,
                                   FX_ARGB argbFill,
                                   const CFX_Matrix& matrix) {
  CFGAS_GEPath path;
  float fBottom = rtSign.bottom();
  float fRadius = (rtSign.top - fBottom) / (1 + cosf(FXSYS_PI / 5.0f));
  CFX_PointF ptCenter((rtSign.left + rtSign.right()) / 2.0f,
                      (rtSign.top + fBottom) / 2.0f);

  std::array<CFX_PointF, 5> points;
  float fAngle = FXSYS_PI / 10.0f;
  for (auto& point : points) {
    point =
        ptCenter + CFX_PointF(fRadius * cosf(fAngle), fRadius * sinf(fAngle));
    fAngle += FXSYS_PI * 2 / 5.0f;
  }

  path.MoveTo(points[0]);
  int next = 0;
  for (size_t i = 0; i < std::size(points); ++i) {
    next = (next + 2) % std::size(points);
    path.LineTo(points[next]);
  }

  CFGAS_GEGraphics::StateRestorer restorer(pGraphics);
  pGraphics->SetFillColor(CFGAS_GEColor(argbFill));
  pGraphics->FillPath(path, CFX_FillRenderOptions::FillType::kWinding, matrix);
}

void CFWL_CheckBoxTP::EnsureCheckPathInitialized(float fCheckLen) {
  if (check_path_) {
    return;
  }

  check_path_ = std::make_unique<CFGAS_GEPath>();

  float fWidth = kSignPath;
  float fHeight = -kSignPath;
  float fBottom = kSignPath;
  CFX_PointF pt1(fWidth / 15.0f, fBottom + fHeight * 2 / 5.0f);
  CFX_PointF pt2(fWidth / 4.5f, fBottom + fHeight / 16.0f);
  CFX_PointF pt3(fWidth / 3.0f, fBottom);
  CFX_PointF pt4(fWidth * 14 / 15.0f, fBottom + fHeight * 15 / 16.0f);
  CFX_PointF pt5(fWidth / 3.6f, fBottom + fHeight / 3.5f);
  CFX_PointF pt12(fWidth / 7.0f, fBottom + fHeight * 2 / 7.0f);
  CFX_PointF pt21(fWidth / 5.0f, fBottom + fHeight / 5.0f);
  CFX_PointF pt23(fWidth / 4.4f, fBottom + fHeight * 0 / 16.0f);
  CFX_PointF pt32(fWidth / 4.0f, fBottom);
  CFX_PointF pt34(fWidth * (1 / 7.0f + 7 / 15.0f),
                  fBottom + fHeight * 4 / 5.0f);
  CFX_PointF pt43(fWidth * (1 / 7.0f + 7 / 15.0f),
                  fBottom + fHeight * 4 / 5.0f);
  CFX_PointF pt45(fWidth * 7 / 15.0f, fBottom + fHeight * 8 / 7.0f);
  CFX_PointF pt54(fWidth / 3.4f, fBottom + fHeight / 3.5f);
  CFX_PointF pt51(fWidth / 3.6f, fBottom + fHeight / 4.0f);
  CFX_PointF pt15(fWidth / 3.5f, fBottom + fHeight * 3.5f / 5.0f);
  check_path_->MoveTo(pt1);

  CFX_PointF p1 = ScaleBezierPoint(pt12 - pt1);
  CFX_PointF p2 = ScaleBezierPoint(pt21 - pt2);
  check_path_->BezierTo(pt1 + p1, pt2 + p2, pt2);

  p1 = ScaleBezierPoint(pt23 - pt2);
  p2 = ScaleBezierPoint(pt32 - pt3);
  check_path_->BezierTo(pt2 + p1, pt3 + p2, pt3);

  p1 = ScaleBezierPoint(pt34 - pt3);
  p2 = ScaleBezierPoint(pt43 - pt4);
  check_path_->BezierTo(pt3 + p1, pt4 + p2, pt4);

  p1 = ScaleBezierPoint(pt45 - pt4);
  p2 = ScaleBezierPoint(pt54 - pt5);
  check_path_->BezierTo(pt4 + p1, pt5 + p2, pt5);

  p1 = ScaleBezierPoint(pt51 - pt5);
  p2 = ScaleBezierPoint(pt15 - pt1);
  check_path_->BezierTo(pt5 + p1, pt1 + p2, pt1);

  float fScale = fCheckLen / kSignPath;
  CFX_Matrix mt;
  mt.Scale(fScale, fScale);
  check_path_->TransformBy(mt);
}

void CFWL_CheckBoxTP::DrawBackground(const CFWL_ThemeBackground& pParams) {
  if (pParams.GetPart() != CFWL_ThemePart::Part::kCheckBox) {
    return;
  }

  if ((pParams.states_ & CFWL_PartState::kChecked) ||
      (pParams.states_ & CFWL_PartState::kNeutral)) {
    DrawCheckSign(pParams.GetWidget(), pParams.GetGraphics(),
                  pParams.part_rect_, pParams.states_, pParams.matrix_);
  }
}

void CFWL_CheckBoxTP::DrawCheckSign(CFWL_Widget* pWidget,
                                    CFGAS_GEGraphics* pGraphics,
                                    const CFX_RectF& pRtBox,
                                    Mask<CFWL_PartState> iState,
                                    const CFX_Matrix& matrix) {
  CFX_RectF rtSign(pRtBox);
  uint32_t dwColor =
      iState & CFWL_PartState::kNeutral ? 0xFFA9A9A9 : 0xFF000000;
  uint32_t dwStyle = pWidget->GetStyleExts();
  rtSign.Deflate(rtSign.width / 4, rtSign.height / 4);
  switch (dwStyle & FWL_STYLEEXT_CKB_SignShapeMask) {
    case FWL_STYLEEXT_CKB_SignShapeCheck:
      DrawSignCheck(pGraphics, rtSign, dwColor, matrix);
      break;
    case FWL_STYLEEXT_CKB_SignShapeCircle:
      DrawSignCircle(pGraphics, rtSign, dwColor, matrix);
      break;
    case FWL_STYLEEXT_CKB_SignShapeCross:
      DrawSignCross(pGraphics, rtSign, dwColor, matrix);
      break;
    case FWL_STYLEEXT_CKB_SignShapeDiamond:
      DrawSignDiamond(pGraphics, rtSign, dwColor, matrix);
      break;
    case FWL_STYLEEXT_CKB_SignShapeSquare:
      DrawSignSquare(pGraphics, rtSign, dwColor, matrix);
      break;
    case FWL_STYLEEXT_CKB_SignShapeStar:
      DrawSignStar(pGraphics, rtSign, dwColor, matrix);
      break;
    default:
      break;
  }
}

}  // namespace pdfium
