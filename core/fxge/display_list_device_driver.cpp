// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/display_list_device_driver.h"

#include <math.h>

#include <utility>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/span.h"
#include "core/fxge/cfx_fillrenderoptions.h"
#include "core/fxge/cfx_font.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/text_char_pos.h"

namespace {

ByteString FormatFloat(float val) {
  if (fabs(val) < 0.00005f) {
    val = 0.0f;
  }
  return ByteString::Format("%.4f", val);
}

ByteString FormatColor(uint32_t color) {
  return ByteString::Format("0x%08x", color);
}

ByteString FormatPoint(const CFX_PointF& pt) {
  return ByteString({'(', FormatFloat(pt.x).AsStringView(), ',',
                     FormatFloat(pt.y).AsStringView(), ')'});
}

ByteString FormatGlyph(const TextCharPos& cp) {
  ByteString prefix =
      cp.unicode_ != 0
          ? ByteString::Format("(idx=%u,u=U+%04X,origin=", cp.glyph_index_,
                               cp.unicode_)
          : ByteString::Format("(idx=%u,origin=", cp.glyph_index_);
  return ByteString(
      {prefix.AsStringView(), FormatPoint(cp.origin_).AsStringView(), ')'});
}

ByteString FormatDIBInfo(const CFX_DIBBase& bitmap) {
  return ByteString::Format("size=[%d,%d] bpp=%d", bitmap.GetWidth(),
                            bitmap.GetHeight(), bitmap.GetBPP());
}

ByteString FormatMatrix(const CFX_Matrix& m) {
  return ByteString({'[', FormatFloat(m.a).AsStringView(), ',',
                     FormatFloat(m.b).AsStringView(), ',',
                     FormatFloat(m.c).AsStringView(), ',',
                     FormatFloat(m.d).AsStringView(), ',',
                     FormatFloat(m.e).AsStringView(), ',',
                     FormatFloat(m.f).AsStringView(), ']'});
}

ByteString FormatPath(const CFX_Path& path) {
  ByteString res;
  for (size_t i = 0; i < path.GetPoints().size(); ++i) {
    if (i > 0) {
      res += " ";
    }
    const CFX_Path::Point& pt = path.GetPoints()[i];
    const char* type_str;
    if (pt.type_ == CFX_Path::Point::Type::kMove) {
      type_str = "move";
    } else if (pt.type_ == CFX_Path::Point::Type::kBezier) {
      type_str = "bezier";
    } else {
      CHECK_EQ(pt.type_, CFX_Path::Point::Type::kLine);
      type_str = "line";
    }
    res += ByteString({type_str, '(', FormatFloat(pt.point_.x).AsStringView(),
                       ',', FormatFloat(pt.point_.y).AsStringView(),
                       pt.close_figure_ ? ",close" : "", ')'});
  }
  return res;
}

const char* FormatFillType(CFX_FillRenderOptions::FillType fill_type) {
  if (fill_type == CFX_FillRenderOptions::FillType::kEvenOdd) {
    return "even-odd";
  }
  if (fill_type == CFX_FillRenderOptions::FillType::kWinding) {
    return "winding";
  }
  CHECK_EQ(fill_type, CFX_FillRenderOptions::FillType::kNoFill);
  return "no-fill";
}

const char* FormatLineCap(CFX_GraphStateData::LineCap line_cap) {
  if (line_cap == CFX_GraphStateData::LineCap::kRound) {
    return "round";
  }
  if (line_cap == CFX_GraphStateData::LineCap::kSquare) {
    return "square";
  }
  CHECK_EQ(line_cap, CFX_GraphStateData::LineCap::kButt);
  return "butt";
}

const char* FormatLineJoin(CFX_GraphStateData::LineJoin line_join) {
  if (line_join == CFX_GraphStateData::LineJoin::kRound) {
    return "round";
  }
  if (line_join == CFX_GraphStateData::LineJoin::kBevel) {
    return "bevel";
  }
  CHECK_EQ(line_join, CFX_GraphStateData::LineJoin::kMiter);
  return "miter";
}

}  // namespace

DisplayListDeviceDriver::DisplayListDeviceDriver(int width, int height)
    : width_(width), height_(height), clip_box_(0, 0, width, height) {}

DisplayListDeviceDriver::~DisplayListDeviceDriver() = default;

DeviceType DisplayListDeviceDriver::GetDeviceType() const {
  return DeviceType::kDisplay;
}

int DisplayListDeviceDriver::GetPixelWidth() const {
  return width_;
}

int DisplayListDeviceDriver::GetPixelHeight() const {
  return height_;
}

int DisplayListDeviceDriver::GetBitsPerPixel() const {
  return 32;
}

bool DisplayListDeviceDriver::RenderCapAlphaOutput() const {
  return true;
}

#if defined(PDF_USE_SKIA)
bool DisplayListDeviceDriver::RenderCapFillStrokePath() const {
  return true;
}
#endif

void DisplayListDeviceDriver::Clear(uint32_t color) {
  display_list_ += ByteString::Format("Clear color=0x%08x\n", color);
}

void DisplayListDeviceDriver::SaveState() {
  clip_stack_.push_back(clip_box_);
  display_list_ += "SaveState\n";
}

void DisplayListDeviceDriver::RestoreState(bool bKeepSaved) {
  if (!clip_stack_.empty()) {
    clip_box_ = clip_stack_.back();
    if (!bKeepSaved) {
      clip_stack_.pop_back();
    }
  }
  display_list_ += "RestoreState\n";
}

void DisplayListDeviceDriver::SetBaseClip(const FX_RECT& rect) {
  clip_box_ = rect;
  display_list_ += ByteString::Format("SetBaseClip [%d,%d,%d,%d]\n", rect.left,
                                      rect.top, rect.right, rect.bottom);
}

bool DisplayListDeviceDriver::SetClip_PathFill(
    const CFX_Path& path,
    const CFX_Matrix* pObject2Device,
    const CFX_FillRenderOptions& fill_options) {
  const char* rule = FormatFillType(fill_options.fill_type);
  ByteString mat_str =
      pObject2Device ? FormatMatrix(*pObject2Device) : ByteString("none");
  display_list_ += ByteString({"SetClip_PathFill rule=", rule,
                               " matrix=", mat_str.AsStringView(), " points=[",
                               FormatPath(path).AsStringView(), "]\n"});
  return true;
}

bool DisplayListDeviceDriver::SetClip_PathStroke(
    const CFX_Path& path,
    const CFX_Matrix* pObject2Device,
    const CFX_GraphStateData* pGraphState) {
  ByteString mat_str =
      pObject2Device ? FormatMatrix(*pObject2Device) : ByteString("none");
  display_list_ +=
      ByteString({"SetClip_PathStroke matrix=", mat_str.AsStringView(),
                  " points=[", FormatPath(path).AsStringView(), "]\n"});
  return true;
}

bool DisplayListDeviceDriver::DrawPath(
    const CFX_Path& path,
    const CFX_Matrix* pObject2Device,
    const CFX_GraphStateData* pGraphState,
    uint32_t fill_color,
    uint32_t stroke_color,
    bool group_knockout,
    const CFX_FillRenderOptions& fill_options) {
  ByteString mat_str =
      pObject2Device ? FormatMatrix(*pObject2Device) : ByteString("none");
  ByteString stroke_str;
  if (pGraphState) {
    const char* cap = FormatLineCap(pGraphState->line_cap());
    const char* join = FormatLineJoin(pGraphState->line_join());
    stroke_str = ByteString::Format("width=%.4f cap=%s join=%s miter=%.4f",
                                    pGraphState->line_width(), cap, join,
                                    pGraphState->miter_limit());
  } else {
    stroke_str = "none";
  }
  const char* rule = FormatFillType(fill_options.fill_type);
  display_list_ +=
      ByteString({"DrawPath fill=", FormatColor(fill_color).AsStringView(),
                  " stroke=", FormatColor(stroke_color).AsStringView(),
                  " rule=", rule, " stroke_state=[", stroke_str.AsStringView(),
                  "] matrix=", mat_str.AsStringView(), " points=[",
                  FormatPath(path).AsStringView(), "]\n"});
  return true;
}

bool DisplayListDeviceDriver::FillRect(const FX_RECT& rect,
                                       uint32_t fill_color) {
  display_list_ +=
      ByteString::Format("FillRect [%d,%d,%d,%d] color=0x%08x\n", rect.left,
                         rect.top, rect.right, rect.bottom, fill_color);
  return true;
}

bool DisplayListDeviceDriver::DrawCosmeticLine(const CFX_PointF& ptMoveTo,
                                               const CFX_PointF& ptLineTo,
                                               uint32_t color) {
  display_list_ += ByteString::Format(
      "DrawCosmeticLine (%.4f,%.4f)->(%.4f,%.4f) color=0x%08x\n", ptMoveTo.x,
      ptMoveTo.y, ptLineTo.x, ptLineTo.y, color);
  return true;
}

FX_RECT DisplayListDeviceDriver::GetClipBox() const {
  return clip_box_;
}

bool DisplayListDeviceDriver::SetDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                                        uint32_t color,
                                        const FX_RECT& src_rect,
                                        int left,
                                        int top,
                                        BlendMode blend_type) {
  display_list_ += ByteString::Format(
      "SetDIBits size=[%d,%d] bpp=%d dest=(%d,%d) src=[%d,%d,%d,%d] "
      "color=0x%08x\n",
      bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetBPP(), left, top,
      src_rect.left, src_rect.top, src_rect.right, src_rect.bottom, color);
  return true;
}

bool DisplayListDeviceDriver::StretchDIBits(
    RetainPtr<const CFX_DIBBase> bitmap,
    uint32_t color,
    int dest_left,
    int dest_top,
    int dest_width,
    int dest_height,
    const FX_RECT* pClipRect,
    const FXDIB_ResampleOptions& options,
    BlendMode blend_type) {
  display_list_ += ByteString::Format(
      "StretchDIBits size=[%d,%d] bpp=%d dest=[%d,%d,%d,%d] color=0x%08x\n",
      bitmap->GetWidth(), bitmap->GetHeight(), bitmap->GetBPP(), dest_left,
      dest_top, dest_width, dest_height, color);
  return true;
}

RenderDeviceDriverIface::StartResult DisplayListDeviceDriver::StartDIBits(
    RetainPtr<const CFX_DIBBase> bitmap,
    float alpha,
    uint32_t color,
    const CFX_Matrix& matrix,
    const FXDIB_ResampleOptions& options,
    BlendMode blend_type) {
  display_list_ +=
      ByteString({"StartDIBits ", FormatDIBInfo(*bitmap).AsStringView(),
                  " matrix=", FormatMatrix(matrix).AsStringView(),
                  " alpha=", FormatFloat(alpha).AsStringView(),
                  " color=", FormatColor(color).AsStringView(), "\n"});
  return StartResult(Result::kSuccess, nullptr);
}

bool DisplayListDeviceDriver::DrawDeviceText(
    pdfium::span<const TextCharPos> pCharPos,
    CFX_Font* font,
    const CFX_Matrix& mtObject2Device,
    float font_size,
    uint32_t color,
    const CFX_TextRenderOptions& options) {
  ByteString font_name = font ? font->GetFamilyName() : ByteString();
  if (font_name.IsEmpty() && font) {
    font_name = font->GetPsName();
  }
  display_list_ += ByteString(
      {"DrawDeviceText font=\"", font_name.AsStringView(),
       "\" size=", FormatFloat(font_size).AsStringView(),
       " color=", FormatColor(color).AsStringView(),
       " matrix=", FormatMatrix(mtObject2Device).AsStringView(), " glyphs=["});

  for (size_t i = 0; i < pCharPos.size(); ++i) {
    if (i > 0) {
      display_list_ += ' ';
    }
    display_list_ += FormatGlyph(pCharPos[i]);
  }
  display_list_ += "]\n";
  return true;
}

bool DisplayListDeviceDriver::MultiplyAlpha(float alpha) {
  display_list_ += ByteString::Format("MultiplyAlpha alpha=%.4f\n", alpha);
  return true;
}

bool DisplayListDeviceDriver::MultiplyAlphaMask(
    RetainPtr<const CFX_DIBitmap> mask) {
  display_list_ += ByteString::Format("MultiplyAlphaMask size=[%d,%d]\n",
                                      mask ? mask->GetWidth() : 0,
                                      mask ? mask->GetHeight() : 0);
  return true;
}
