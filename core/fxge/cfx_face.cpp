// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_face.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "core/fxcrt/check.h"
#include "core/fxcrt/check_op.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxcrt/notreached.h"
#include "core/fxcrt/numerics/clamped_math.h"
#include "core/fxcrt/numerics/safe_conversions.h"
#include "core/fxcrt/numerics/safe_math.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/to_underlying.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/cfx_cttgsubtable.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/cfx_fontmgr.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_glyphbitmap.h"
#include "core/fxge/cfx_glyphcache.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/cfx_substfont.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"

#if defined(PDF_ENABLE_XFA)
#include "core/fxge/cfx_cttnametable.h"
#endif  // defined(PDF_ENABLE_XFA)

#if defined(PDF_USE_SKIA)
#include "third_party/skia/include/core/SkTypeface.h"  // nogncheck
#endif

#if defined(PDF_ENABLE_FONTATIONS)
#include "core/fxge/skrifa/src/main.rs.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"
#include "third_party/rust/chromium_crates_io/vendor/cxx-v1/include/cxx.h"
#endif

namespace {

#if defined(PDF_ENABLE_FREETYPE)
constexpr int kThousandthMinInt = std::numeric_limits<int>::min() / 1000;
constexpr int kThousandthMaxInt = std::numeric_limits<int>::max() / 1000;
constexpr int kMaxGlyphDimension = 2048;
#endif  // defined(PDF_ENABLE_FREETYPE)

// Boundary value to avoid integer overflow when adding 1/64th of the value.
constexpr int kMaxRectTop = 2114445437;

#if defined(PDF_ENABLE_FREETYPE)
struct OUTLINE_PARAMS {
  UnownedPtr<CFX_Path> path_;
  FT_Pos cur_x_;
  FT_Pos cur_y_;
};

constexpr float kCoordUnit = 64 * 64.0f;

int FTPosToCBoxInt(FT_Pos pos) {
  // Boundary values to avoid integer overflow when multiplied by 1000.
  static constexpr FT_Pos kMinCBox = -2147483;
  static constexpr FT_Pos kMaxCBox = 2147483;
  return static_cast<int>(std::clamp(pos, kMinCBox, kMaxCBox));
}

void Outline_CheckEmptyContour(OUTLINE_PARAMS* param) {
  size_t size;
  {
    pdfium::span<const CFX_Path::Point> points = param->path_->GetPoints();
    size = points.size();

    if (size >= 2 &&
        points[size - 2].IsTypeAndOpen(CFX_Path::Point::Type::kMove) &&
        points[size - 2].point_ == points[size - 1].point_) {
      size -= 2;
    }
    if (size >= 4 &&
        points[size - 4].IsTypeAndOpen(CFX_Path::Point::Type::kMove) &&
        points[size - 3].IsTypeAndOpen(CFX_Path::Point::Type::kBezier) &&
        points[size - 3].point_ == points[size - 4].point_ &&
        points[size - 2].point_ == points[size - 4].point_ &&
        points[size - 1].point_ == points[size - 4].point_) {
      size -= 4;
    }
  }
  // Only safe after |points| has been destroyed.
  param->path_->GetPoints().resize(size);
}

int Outline_MoveTo(const FT_Vector* to, void* user) {
  OUTLINE_PARAMS* param = static_cast<OUTLINE_PARAMS*>(user);

  Outline_CheckEmptyContour(param);

  param->path_->ClosePath();
  param->path_->AppendPoint(CFX_PointF(to->x / kCoordUnit, to->y / kCoordUnit),
                            CFX_Path::Point::Type::kMove);

  param->cur_x_ = to->x;
  param->cur_y_ = to->y;
  return 0;
}

int Outline_LineTo(const FT_Vector* to, void* user) {
  OUTLINE_PARAMS* param = static_cast<OUTLINE_PARAMS*>(user);

  param->path_->AppendPoint(CFX_PointF(to->x / kCoordUnit, to->y / kCoordUnit),
                            CFX_Path::Point::Type::kLine);

  param->cur_x_ = to->x;
  param->cur_y_ = to->y;
  return 0;
}

int Outline_ConicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
  OUTLINE_PARAMS* param = static_cast<OUTLINE_PARAMS*>(user);

  param->path_->AppendPoint(
      CFX_PointF(
          (param->cur_x_ + (control->x - param->cur_x_) * 2 / 3) / kCoordUnit,
          (param->cur_y_ + (control->y - param->cur_y_) * 2 / 3) / kCoordUnit),
      CFX_Path::Point::Type::kBezier);

  param->path_->AppendPoint(
      CFX_PointF((control->x + (to->x - control->x) / 3) / kCoordUnit,
                 (control->y + (to->y - control->y) / 3) / kCoordUnit),
      CFX_Path::Point::Type::kBezier);

  param->path_->AppendPoint(CFX_PointF(to->x / kCoordUnit, to->y / kCoordUnit),
                            CFX_Path::Point::Type::kBezier);

  param->cur_x_ = to->x;
  param->cur_y_ = to->y;
  return 0;
}

int Outline_CubicTo(const FT_Vector* control1,
                    const FT_Vector* control2,
                    const FT_Vector* to,
                    void* user) {
  OUTLINE_PARAMS* param = static_cast<OUTLINE_PARAMS*>(user);

  param->path_->AppendPoint(
      CFX_PointF(control1->x / kCoordUnit, control1->y / kCoordUnit),
      CFX_Path::Point::Type::kBezier);

  param->path_->AppendPoint(
      CFX_PointF(control2->x / kCoordUnit, control2->y / kCoordUnit),
      CFX_Path::Point::Type::kBezier);

  param->path_->AppendPoint(CFX_PointF(to->x / kCoordUnit, to->y / kCoordUnit),
                            CFX_Path::Point::Type::kBezier);

  param->cur_x_ = to->x;
  param->cur_y_ = to->y;
  return 0;
}
#endif  // defined(PDF_ENABLE_FREETYPE)

#if defined(PDF_ENABLE_FONTATIONS)
constexpr float kFixedPpem = 64.0f;

CFX_PointF ToCFXPointF(const skrifa::Point& pt) {
  return CFX_PointF(pt.x, pt.y);
}

std::unique_ptr<CFX_Path> ConvertOutline(const skrifa::Outline& outline) {
  if (outline.verbs.empty() || outline.points.empty()) {
    return nullptr;
  }
  auto skrifa_path = std::make_unique<CFX_Path>();
  size_t point_idx = 0;
  CFX_PointF current_point(0, 0);
  for (auto verb : outline.verbs) {
    switch (verb) {
      case skrifa::PathVerb::MoveTo: {
        if (point_idx >= outline.points.size()) {
          return nullptr;
        }
        current_point = ToCFXPointF(outline.points[point_idx++]);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kMove);
        break;
      }
      case skrifa::PathVerb::LineTo: {
        if (point_idx >= outline.points.size()) {
          return nullptr;
        }
        current_point = ToCFXPointF(outline.points[point_idx++]);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kLine);
        break;
      }
      case skrifa::PathVerb::QuadTo: {
        if (point_idx + 1 >= outline.points.size()) {
          return nullptr;
        }
        CFX_PointF c0 = ToCFXPointF(outline.points[point_idx++]);
        skrifa_path->AppendPoint(
            CFX_PointF(current_point.x + (c0.x - current_point.x) * 2 / 3,
                       current_point.y + (c0.y - current_point.y) * 2 / 3),
            CFX_Path::Point::Type::kBezier);
        current_point = ToCFXPointF(outline.points[point_idx++]);
        skrifa_path->AppendPoint(
            CFX_PointF(c0.x + (current_point.x - c0.x) / 3,
                       c0.y + (current_point.y - c0.y) / 3),
            CFX_Path::Point::Type::kBezier);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kBezier);
        break;
      }
      case skrifa::PathVerb::CurveTo: {
        if (point_idx + 2 >= outline.points.size()) {
          return nullptr;
        }
        CFX_PointF c0 = ToCFXPointF(outline.points[point_idx++]);
        CFX_PointF c1 = ToCFXPointF(outline.points[point_idx++]);
        current_point = ToCFXPointF(outline.points[point_idx++]);
        skrifa_path->AppendPoint(c0, CFX_Path::Point::Type::kBezier);
        skrifa_path->AppendPoint(c1, CFX_Path::Point::Type::kBezier);
        skrifa_path->AppendPoint(current_point, CFX_Path::Point::Type::kBezier);
        break;
      }
      case skrifa::PathVerb::Close:
        skrifa_path->ClosePath();
        break;
    }
  }
  return skrifa_path;
}

#if defined(PDF_ENABLE_FREETYPE)
void CloseContours(pdfium::span<FT_Vector> points,
                   std::vector<uint16_t>& contours) {
  if (!points.empty()) {
    uint16_t last = static_cast<uint16_t>(points.size() - 1);
    if (contours.empty() || contours.back() != last) {
      contours.push_back(last);
    }
  }
}

struct ConvertedFTOutline {
  std::vector<FT_Vector> points;
  std::vector<unsigned char> tags;
  std::vector<unsigned short> contours;
};

FT_Vector ToFTVector(const skrifa::Point& p, const CFX_Matrix& scaled_matrix) {
  CFX_PointF pt = scaled_matrix.Transform(ToCFXPointF(p));
  return {static_cast<FT_Pos>(std::round(pt.x)),
          static_cast<FT_Pos>(std::round(pt.y))};
}

ConvertedFTOutline ConvertToFTOutline(const skrifa::Outline& outline,
                                      const CFX_Matrix& matrix,
                                      float scale,
                                      float x_scale) {
  const CFX_Matrix scaled_matrix(matrix.a * scale * x_scale, matrix.b * scale,
                                 matrix.c * scale * x_scale, matrix.d * scale,
                                 0, 0);
  ConvertedFTOutline result;
  size_t point_idx = 0;
  for (auto verb : outline.verbs) {
    switch (verb) {
      case skrifa::PathVerb::MoveTo: {
        if (point_idx >= outline.points.size()) {
          break;
        }
        CloseContours(result.points, result.contours);
        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_ON);
        break;
      }
      case skrifa::PathVerb::LineTo: {
        if (point_idx >= outline.points.size()) {
          break;
        }
        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_ON);
        break;
      }
      case skrifa::PathVerb::QuadTo: {
        if (point_idx + 1 >= outline.points.size()) {
          break;
        }
        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_CONIC);

        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_ON);
        break;
      }
      case skrifa::PathVerb::CurveTo: {
        if (point_idx + 2 >= outline.points.size()) {
          break;
        }
        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_CUBIC);

        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_CUBIC);

        result.points.push_back(
            ToFTVector(outline.points[point_idx++], scaled_matrix));
        result.tags.push_back(FT_CURVE_TAG_ON);
        break;
      }
      case skrifa::PathVerb::Close:
        CloseContours(result.points, result.contours);
        break;
    }
  }
  CloseContours(result.points, result.contours);
  return result;
}
#endif  // defined(PDF_ENABLE_FREETYPE)
#endif  // defined(PDF_ENABLE_FONTATIONS)

#if defined(PDF_ENABLE_FREETYPE)
FT_Encoding ToFTEncoding(fxge::FontEncoding encoding) {
  switch (encoding) {
    case fxge::FontEncoding::kAdobeCustom:
      return FT_ENCODING_ADOBE_CUSTOM;
    case fxge::FontEncoding::kAdobeExpert:
      return FT_ENCODING_ADOBE_EXPERT;
    case fxge::FontEncoding::kAdobeStandard:
      return FT_ENCODING_ADOBE_STANDARD;
    case fxge::FontEncoding::kAppleRoman:
      return FT_ENCODING_APPLE_ROMAN;
    case fxge::FontEncoding::kBig5:
      return FT_ENCODING_BIG5;
    case fxge::FontEncoding::kGB2312:
      return FT_ENCODING_PRC;
    case fxge::FontEncoding::kJohab:
      return FT_ENCODING_JOHAB;
    case fxge::FontEncoding::kLatin1:
      return FT_ENCODING_ADOBE_LATIN_1;
    case fxge::FontEncoding::kNone:
      return FT_ENCODING_NONE;
    case fxge::FontEncoding::kOldLatin2:
      return FT_ENCODING_OLD_LATIN_2;
    case fxge::FontEncoding::kSjis:
      return FT_ENCODING_SJIS;
    case fxge::FontEncoding::kSymbol:
      return FT_ENCODING_MS_SYMBOL;
    case fxge::FontEncoding::kUnicode:
      return FT_ENCODING_UNICODE;
    case fxge::FontEncoding::kWansung:
      return FT_ENCODING_WANSUNG;
  }
}
#endif  // defined(PDF_ENABLE_FREETYPE)

fxge::FontEncoding CharMapIdPairToFontEncoding(
    CFX_Face::CharMapIdPair charmap_id_pair) {
  const auto [platform_id, encoding_id] = charmap_id_pair;
  if (platform_id == kPlatformAppleUnicode) {
    return fxge::FontEncoding::kUnicode;
  }
  if (platform_id == kPlatformMac) {
    if (encoding_id == kMacEncodingRoman) {
      return fxge::FontEncoding::kAppleRoman;
    }
    return fxge::FontEncoding::kNone;
  }
  if (platform_id == kPlatformIso) {
    // FreeType treats all encodings under TT_PLATFORM_ISO as Unicode.
    return fxge::FontEncoding::kUnicode;
  }
  if (platform_id == kPlatformWindows) {
    switch (encoding_id) {
      case kWindowsEncodingSymbol:
        return fxge::FontEncoding::kSymbol;
      case kWindowsEncodingUnicode:
      case kWindowsEncodingUcs4:
        return fxge::FontEncoding::kUnicode;
      case kWindowsEncodingSjis:
        return fxge::FontEncoding::kSjis;
      case kWindowsEncodingGb2312:
        return fxge::FontEncoding::kGB2312;
      case kWindowsEncodingBig5:
        return fxge::FontEncoding::kBig5;
      case kWindowsEncodingWansung:
        return fxge::FontEncoding::kWansung;
      case kWindowsEncodingJohab:
        return fxge::FontEncoding::kJohab;
    }
  }
  if (platform_id == kPlatformAdobe) {
    switch (encoding_id) {
      case kAdobeEncodingStandard:
        return fxge::FontEncoding::kAdobeStandard;
      case kAdobeEncodingExpert:
        return fxge::FontEncoding::kAdobeExpert;
      case kAdobeEncodingCustom:
        return fxge::FontEncoding::kAdobeCustom;
      case kAdobeEncodingLatin1:
        return fxge::FontEncoding::kLatin1;
    }
  }
  return fxge::FontEncoding::kNone;
}

#if defined(PDF_ENABLE_FREETYPE)
FX_RECT FXRectFromFTPos(FT_Pos left, FT_Pos top, FT_Pos right, FT_Pos bottom) {
  return FX_RECT(pdfium::checked_cast<int32_t>(left),
                 pdfium::checked_cast<int32_t>(top),
                 pdfium::checked_cast<int32_t>(right),
                 pdfium::checked_cast<int32_t>(bottom));
}

FX_RECT ScaledFXRectFromFTPos(FT_Pos left,
                              FT_Pos top,
                              FT_Pos right,
                              FT_Pos bottom,
                              int x_scale,
                              int y_scale) {
  if (x_scale == 0 || y_scale == 0) {
    return FXRectFromFTPos(left, top, right, bottom);
  }

  return FXRectFromFTPos(left * 1000 / x_scale, top * 1000 / y_scale,
                         right * 1000 / x_scale, bottom * 1000 / y_scale);
}

FT_Render_Mode FtRenderModeFromFontAntiAliasingMode(
    FontAntiAliasingMode anti_alias) {
  switch (anti_alias) {
    case FontAntiAliasingMode::kNormal:
      return FT_RENDER_MODE_NORMAL;
    case FontAntiAliasingMode::kMono:
      return FT_RENDER_MODE_MONO;
    case FontAntiAliasingMode::kLcd:
      return FT_RENDER_MODE_LCD;
  }
  NOTREACHED();
}

// Sets the given transform on the FaceRec, and resets it to the identity when
// it goes out of scope.
class ScopedFaceTransform {
 public:
  FX_STACK_ALLOCATED();

  ScopedFaceTransform(FT_FaceRec* rec, FT_Matrix* matrix) : rec_(rec) {
    FT_Set_Transform(rec_, matrix, nullptr);
  }

  ~ScopedFaceTransform() {
    FT_Matrix matrix = {0x10000L, 0L, 0L, 0x10000L};
    FT_Set_Transform(rec_, &matrix, nullptr);
  }

 private:
  UnownedPtr<FT_FaceRec> const rec_;
};
#endif  // defined(PDF_ENABLE_FREETYPE)

}  // namespace

#if defined(PDF_ENABLE_FONTATIONS)
struct SkrifaFontHolder {
  explicit SkrifaFontHolder(rust::Box<skrifa::SkrifaFont> f)
      : font(std::move(f)) {}
  rust::Box<skrifa::SkrifaFont> font;
};
#endif  // defined(PDF_ENABLE_FONTATIONS)

// static
RetainPtr<CFX_Face> CFX_Face::New(RetainPtr<Retainable> cache_entry,
                                  RetainPtr<CFX_ReadOnlySpanStream> font_stream,
                                  uint32_t face_index) {
  pdfium::span<const uint8_t> data = font_stream->span();

#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    auto raw_font =
        skrifa::new_font(rust::Slice<const uint8_t>(data), face_index);
    if (!raw_font->is_ok()) {
      // Everything guarded by CFX_GEModule::IsFontations() dereferences
      // `skrifa_font_`, and this backend does not fall back to FreeType, so a
      // font that Fontations cannot parse is of no use. Reject it rather than
      // keeping a face that only FreeType can read.
      return nullptr;
    }
    return pdfium::WrapRetain(
        new CFX_Face(std::move(cache_entry), std::move(font_stream), nullptr,
                     new SkrifaFontHolder(std::move(raw_font))));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)

#if defined(PDF_ENABLE_FREETYPE)
  if (CFX_GEModule::IsFreetype()) {
    CFX_FontMgr* font_mgr = CFX_GEModule::Get()->GetFontMgr();
    FT_FaceRec* face_rec = nullptr;
    if (FT_New_Memory_Face(font_mgr->GetFTLibrary(), data.data(),
                           pdfium::checked_cast<FT_Long>(data.size()),
                           pdfium::checked_cast<FT_Long>(face_index),
                           &face_rec) != 0) {
      return nullptr;
    }
    if (FT_Set_Pixel_Sizes(face_rec, 64, 64) != 0) {
      return nullptr;
    }
    return pdfium::WrapRetain(new CFX_Face(std::move(cache_entry),
                                           std::move(font_stream), face_rec,
                                           /*skrifa_font=*/nullptr));
  }
#endif  // defined(PDF_ENABLE_FREETYPE)

  return nullptr;
}

// static
wchar_t CFX_Face::UnicodeFromAdobeName(const char* name) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    uint32_t unicode = 0;
    return skrifa::agl_name_to_unicode(name, unicode)
               ? static_cast<wchar_t>(unicode)
               : 0;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return static_cast<wchar_t>(FXFT_unicode_from_adobe_name(name) & 0x7FFFFFFF);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

// static
ByteString CFX_Face::AdobeNameFromUnicode(wchar_t unicode) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    std::array<uint8_t, 64> skrifa_glyph_name;
    auto result =
        skrifa::agl_unicode_to_name(static_cast<uint32_t>(unicode),
                                    rust::Slice<uint8_t>(skrifa_glyph_name));
    return ByteString(ByteStringView(result));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  char glyph_name[64];
  FXFT_adobe_name_from_unicode(glyph_name, unicode);
  return ByteString(glyph_name);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

bool CFX_Face::HasGlyphNames() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->has_glyph_names();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return !!(GetRec()->face_flags & FT_FACE_FLAG_GLYPH_NAMES);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

bool CFX_Face::IsTtOt() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->is_sfnt();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  const FT_FaceRec* rec = GetRec();
  return rec && (rec->face_flags & FT_FACE_FLAG_SFNT);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

ByteString CFX_Face::GetFontFormat() {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    switch (skrifa_font_->font->font_type()) {
      case skrifa::FaceFormat::TrueType:
        return "TrueType";
      case skrifa::FaceFormat::Type1:
        return "Type 1";
      case skrifa::FaceFormat::Cff:
        return "CFF";
      default:
        return "";
    }
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return ByteString(FT_Get_Font_Format(GetRec()));
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

bool CFX_Face::IsTricky() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->is_tricky();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return !!(GetRec()->face_flags & FT_FACE_FLAG_TRICKY);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

#if defined(PDF_ENABLE_FONTATIONS)
bool CFX_Face::IsPostScriptFont() const {
  if (!skrifa_font_) {
    return false;
  }
  skrifa::FaceFormat format = skrifa_font_->font->font_type();
  return format == skrifa::FaceFormat::Type1 ||
         format == skrifa::FaceFormat::Cff;
}
#endif  // defined(PDF_ENABLE_FONTATIONS)

bool CFX_Face::IsFixedWidth() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->is_fixed_pitch();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return !!(GetRec()->face_flags & FT_FACE_FLAG_FIXED_WIDTH);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

#if defined(PDF_ENABLE_XFA)
bool CFX_Face::IsScalable() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->is_scalable();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return !!(GetRec()->face_flags & FT_FACE_FLAG_SCALABLE);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}
#endif  // defined(PDF_ENABLE_XFA)

bool CFX_Face::IsItalic() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->is_italic();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  const FT_FaceRec* rec = GetRec();
  return rec && (rec->style_flags & FT_STYLE_FLAG_ITALIC);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

bool CFX_Face::IsBold() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->is_bold();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  const FT_FaceRec* rec = GetRec();
  return rec && (rec->style_flags & FT_STYLE_FLAG_BOLD);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

ByteString CFX_Face::GetFamilyName() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    rust::Str skrifa_result = skrifa_font_->font->family_name();
    return ByteString(ByteStringView(skrifa_result));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return ByteString(GetRec()->family_name);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

ByteString CFX_Face::GetStyleName() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    rust::String skrifa_result = skrifa_font_->font->style_name();
    return ByteString(skrifa_result.c_str());
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return ByteString(GetRec()->style_name);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

FX_RECT CFX_Face::GetBBox() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    skrifa::BoundingBox bbox;
    if (skrifa_font_->font->get_font_bbox(bbox)) {
      return FX_RECT(pdfium::checked_cast<int32_t>(std::round(bbox.x_min)),
                     pdfium::checked_cast<int32_t>(std::round(bbox.y_min)),
                     pdfium::checked_cast<int32_t>(std::round(bbox.x_max)),
                     pdfium::checked_cast<int32_t>(std::round(bbox.y_max)));
    }
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (CFX_GEModule::IsFreetype()) {
    const FT_FaceRec* rec = GetRec();
    if (rec) {
      return FX_RECT(pdfium::checked_cast<int32_t>(rec->bbox.xMin),
                     pdfium::checked_cast<int32_t>(rec->bbox.yMin),
                     pdfium::checked_cast<int32_t>(rec->bbox.xMax),
                     pdfium::checked_cast<int32_t>(rec->bbox.yMax));
    }
  }
#endif  // defined(PDF_ENABLE_FREETYPE)
  return FX_RECT(-1000, -1000, 1000, 1000);
}

uint16_t CFX_Face::GetUnitsPerEm() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return pdfium::checked_cast<uint16_t>(skrifa_font_->font->units_per_em());
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return pdfium::checked_cast<uint16_t>(GetRec()->units_per_EM);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

int CFX_Face::EmAdjust(int value) const {
  return GetUnitsPerEm() == 0 ? value : value * 1000 / GetUnitsPerEm();
}

int16_t CFX_Face::GetAscender() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return static_cast<int16_t>(std::round(skrifa_font_->font->ascent()));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return pdfium::checked_cast<int16_t>(GetRec()->ascender);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

int16_t CFX_Face::GetDescender() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return static_cast<int16_t>(std::round(skrifa_font_->font->descent()));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return pdfium::checked_cast<int16_t>(GetRec()->descender);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

pdfium::span<const uint8_t> CFX_Face::GetData() const {
  return font_stream_->span();
}

size_t CFX_Face::GetSfntTable(uint32_t table, pdfium::span<uint8_t> buffer) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->get_sfnt_table(table,
                                              rust::Slice<uint8_t>(buffer));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return 0;
  }
  unsigned long length = pdfium::checked_cast<unsigned long>(buffer.size());
  if (length) {
    int error = FT_Load_Sfnt_Table(GetRec(), table, 0, buffer.data(), &length);
    if (!error && length == buffer.size()) {
      return buffer.size();
    }
  } else {
    int error = FT_Load_Sfnt_Table(GetRec(), table, 0, nullptr, &length);
    if (!error && length) {
      return pdfium::checked_cast<size_t>(length);
    }
  }
#endif  // defined(PDF_ENABLE_FREETYPE)
  return 0;
}

std::unique_ptr<CFX_CTTGSUBTable> CFX_Face::ParseGSUBTable() {
  static constexpr uint32_t kGsubTag =
      CFX_FontMapper::MakeTag('G', 'S', 'U', 'B');
  size_t length = GetSfntTable(kGsubTag, {});
  if (!length) {
    return nullptr;
  }
  auto sub_data = FixedSizeDataVector<uint8_t>::Uninit(length);
  if (!GetSfntTable(kGsubTag, sub_data.span())) {
    return nullptr;
  }
  // CFX_CTTGSUBTable parses the data and stores all the values in its structs.
  // It does not store pointers into `sub_data`.
  return std::make_unique<CFX_CTTGSUBTable>(sub_data.span());
}

#if defined(PDF_ENABLE_XFA)
std::unique_ptr<CFX_CTTNameTable> CFX_Face::ParseNameTable() {
  static constexpr uint32_t kNameTag =
      CFX_FontMapper::MakeTag('n', 'a', 'm', 'e');
  size_t length = GetSfntTable(kNameTag, {});
  if (!length) {
    return nullptr;
  }
  auto name_data = FixedSizeDataVector<uint8_t>::Uninit(length);
  if (!GetSfntTable(kNameTag, name_data.span())) {
    return nullptr;
  }
  return std::make_unique<CFX_CTTNameTable>(name_data.span());
}

std::optional<std::array<uint32_t, 4>> CFX_Face::GetOs2UnicodeRange() {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    skrifa::UnicodeRange range;
    if (skrifa_font_->font->get_os2_unicode_range(range)) {
      return std::array<uint32_t, 4>{range.range1, range.range2, range.range3,
                                     range.range4};
    }
    return std::nullopt;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return std::nullopt;
  }
  auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(GetRec(), FT_SFNT_OS2));
  if (!os2) {
    return std::nullopt;
  }
  return std::array<uint32_t, 4>{static_cast<uint32_t>(os2->ulUnicodeRange1),
                                 static_cast<uint32_t>(os2->ulUnicodeRange2),
                                 static_cast<uint32_t>(os2->ulUnicodeRange3),
                                 static_cast<uint32_t>(os2->ulUnicodeRange4)};
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}
#endif  // defined(PDF_ENABLE_XFA)

#if defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_LINUX)
std::optional<std::array<uint32_t, 2>> CFX_Face::GetOs2CodePageRange() {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    skrifa::CodePageRange range;
    if (skrifa_font_->font->get_os2_code_page_range(range)) {
      return std::array<uint32_t, 2>{range.range1, range.range2};
    }
    return std::nullopt;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return std::nullopt;
  }
  auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(GetRec(), FT_SFNT_OS2));
  if (!os2) {
    return std::nullopt;
  }
  return std::array<uint32_t, 2>{static_cast<uint32_t>(os2->ulCodePageRange1),
                                 static_cast<uint32_t>(os2->ulCodePageRange2)};
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

std::optional<std::array<uint8_t, 2>> CFX_Face::GetOs2Panose() {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    skrifa::Os2Panose panose;
    if (skrifa_font_->font->get_os2_panose(panose)) {
      return std::array<uint8_t, 2>{panose.b0, panose.b1};
    }
    return std::nullopt;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return std::nullopt;
  }
  auto* os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(GetRec(), FT_SFNT_OS2));
  if (!os2) {
    return std::nullopt;
  }
  return std::array<uint8_t, 2>{os2->panose[0], os2->panose[1]};
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}
#endif  // defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID) ||
        // BUILDFLAG(IS_LINUX)

int CFX_Face::GetGlyphCount() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return static_cast<int>(skrifa_font_->font->num_glyphs());
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return pdfium::checked_cast<int>(GetRec()->num_glyphs);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

std::unique_ptr<CFX_GlyphBitmap> CFX_Face::RenderGlyph(
    uint32_t glyph_index,
    bool is_cid_font,
    bool is_vertical,
    const CFX_Matrix& matrix,
    int dest_width,
    FontAntiAliasingMode anti_alias,
    const CFX_SubstFont* subst_font) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (!skrifa_font_ || !skrifa_font_->font->is_ok()) {
      return nullptr;
    }
    skrifa::Outline outline;
    bool is_scaled = IsTtOt() && skrifa_font_->font->hinted_outline(
                                     glyph_index, kFixedPpem,
                                     /*is_pedantic=*/false, outline);
    if (!is_scaled &&
        !skrifa_font_->font->unscaled_outline(glyph_index, outline)) {
      return nullptr;
    }
    const int upem = GetUnitsPerEm();
    if (upem <= 0) {
      return nullptr;
    }
    const float unscaled_advance =
        is_scaled ? outline.advance_width * upem / kFixedPpem
                  : outline.advance_width;
    CFX_Matrix effective_matrix = matrix;
    AdjustSubstFontTransform(subst_font, dest_width, unscaled_advance,
                             is_cid_font, is_vertical, &effective_matrix);
#if defined(PDF_ENABLE_FREETYPE)
    const float scale = is_scaled ? 1.0f : kFixedPpem / upem;
    const bool is_lcd = (anti_alias == FontAntiAliasingMode::kLcd);
    const float x_scale = is_lcd ? 3.0f : 1.0f;
    ConvertedFTOutline converted =
        ConvertToFTOutline(outline, effective_matrix, scale, x_scale);
    if (converted.points.empty() || converted.contours.empty()) {
      return nullptr;
    }

    FT_Outline ft_outline;
    ft_outline.n_points = static_cast<short>(converted.points.size());
    ft_outline.n_contours = static_cast<short>(converted.contours.size());
    ft_outline.points = converted.points.data();
    ft_outline.tags = converted.tags.data();
    ft_outline.contours = converted.contours.data();
    ft_outline.flags = FT_OUTLINE_SMART_DROPOUTS;

    if (subst_font) {
      int32_t ft_matrix_xx = static_cast<int32_t>(matrix.a * 1024.0f);
      int32_t ft_matrix_xy = static_cast<int32_t>(matrix.c * 1024.0f);
      int skew = subst_font->GetEffectiveSkew(is_cid_font);
      if (skew && !is_vertical) {
        ft_matrix_xy -= ft_matrix_xx * skew / 100;
      }
      int level = subst_font->GetEmboldenLevelForRender(
          is_cid_font, ft_matrix_xx, ft_matrix_xy);
      if (level < 0) {
        return nullptr;
      }
      if (level > 0) {
        int x_strength = is_lcd ? level * 3 : level;
        FT_Outline_EmboldenXY(&ft_outline, x_strength, level);
      }
    }

    FT_BBox cbox;
    FT_Outline_Get_CBox(&ft_outline, &cbox);
    int x_left;
    int y_bottom;
    int x_right;
    int y_top;
    if (anti_alias == FontAntiAliasingMode::kMono) {
      // Match FreeType's ft_glyphslot_preset_bitmap() monochrome rounding.
      // Coordinates in `cbox` are in 26.6 fixed-point (1/64th pixel). Round
      // asymmetrically (+31 / +32) so pixel centers (at 32/64) covered by the
      // outline are included. If rounding causes the box to collapse to zero,
      // expand by 1 pixel in the direction of the fractional remainder.
      x_left = static_cast<int>((cbox.xMin + 31) >> 6);
      x_right = static_cast<int>((cbox.xMax + 32) >> 6);
      if (x_left == x_right) {
        if (((cbox.xMin + 31) & 63) - 31 + ((cbox.xMax + 32) & 63) - 32 < 0) {
          --x_left;
        } else {
          ++x_right;
        }
      }
      y_bottom = static_cast<int>((cbox.yMin + 31) >> 6);
      y_top = static_cast<int>((cbox.yMax + 32) >> 6);
      if (y_bottom == y_top) {
        if (((cbox.yMin + 31) & 63) - 31 + ((cbox.yMax + 32) & 63) - 32 < 0) {
          --y_bottom;
        } else {
          ++y_top;
        }
      }
    } else {
      x_left = static_cast<int>(cbox.xMin >> 6);
      y_bottom = static_cast<int>(cbox.yMin >> 6);
      x_right = static_cast<int>((cbox.xMax + 63) >> 6);
      y_top = static_cast<int>((cbox.yMax + 63) >> 6);
    }
    int width = x_right - x_left;
    int height = y_top - y_bottom;
    if (width <= 0 || height <= 0) {
      return nullptr;
    }

    int dib_width;
    int bitmap_left;
    int bitmap_top = y_top;
    if (is_lcd) {
      // Pad by 2/3 of a pixel (2 subpixels, or 128 in 3x 26.6 fixed-point) on
      // each side to accommodate the 5-tap LCD filter, matching FreeType's
      // ft_lcd_padding() and ft_glyphslot_preset_bitmap().
      int normal_left =
          static_cast<int>(std::floor((cbox.xMin - 128) / 192.0f));
      int normal_right =
          static_cast<int>(std::floor((cbox.xMax + 128 + 191) / 192.0f));
      int normal_width = normal_right - normal_left;
      if (normal_width <= 0) {
        return nullptr;
      }
      dib_width = normal_width * 3;
      bitmap_left = normal_left;
      FT_Outline_Translate(&ft_outline, -normal_left * 3 * 64, -y_bottom * 64);
    } else {
      dib_width = width;
      bitmap_left = x_left;
      FT_Outline_Translate(&ft_outline, -x_left * 64, -y_bottom * 64);
    }

    if (dib_width > kMaxGlyphDimension || height > kMaxGlyphDimension) {
      return nullptr;
    }

    const FXDIB_Format format = (anti_alias == FontAntiAliasingMode::kMono)
                                    ? FXDIB_Format::k1bppMask
                                    : FXDIB_Format::k8bppMask;
    RetainPtr<CFX_DIBitmap> new_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
    if (!new_bitmap->Create(dib_width, height, format)) {
      return nullptr;
    }
    new_bitmap->Clear(0);

    FT_Bitmap ft_bitmap = {};
    ft_bitmap.rows = height;
    ft_bitmap.width = dib_width;
    ft_bitmap.pitch = new_bitmap->GetPitch();
    ft_bitmap.buffer = new_bitmap->GetWritableBuffer().data();
    ft_bitmap.num_grays = 256;
    ft_bitmap.pixel_mode = (anti_alias == FontAntiAliasingMode::kMono)
                               ? FT_PIXEL_MODE_MONO
                               : FT_PIXEL_MODE_GRAY;

    CFX_FontMgr* font_mgr = CFX_GEModule::Get()->GetFontMgr();
    int error = FT_Outline_Get_Bitmap(font_mgr->GetFTLibrary(), &ft_outline,
                                      &ft_bitmap);
    if (error) {
      return nullptr;
    }

    if (is_lcd) {
      // Apply FreeType's FT_LCD_FILTER_DEFAULT 5-tap filter across each
      // scanline, matching ft_smooth_lcd_spans().
      static constexpr std::array<uint8_t, 5> kLcdFilterWeights = {8, 77, 86,
                                                                   77, 8};
      std::vector<uint8_t> raw_row(dib_width);
      for (int row = 0; row < height; ++row) {
        pdfium::span<uint8_t> scanline =
            new_bitmap->GetWritableScanline(row).first(
                static_cast<size_t>(dib_width));
        fxcrt::Copy(scanline, raw_row);
        std::ranges::fill(scanline, 0);
        for (int x = 0; x < dib_width; ++x) {
          uint8_t coverage = raw_row[x];
          if (coverage == 0) {
            continue;
          }
          for (int i = 0; i < static_cast<int>(kLcdFilterWeights.size()); ++i) {
            int dst_x = x + i - 2;
            if (dst_x >= 0 && dst_x < dib_width) {
              scanline[dst_x] = pdfium::saturated_cast<uint8_t>(
                  scanline[dst_x] +
                  ((coverage * kLcdFilterWeights[i] + 85) >> 8));
            }
          }
        }
      }
    }

    return std::make_unique<CFX_GlyphBitmap>(CFX_Point(bitmap_left, bitmap_top),
                                             std::move(new_bitmap));
#else
    return nullptr;
#endif  // defined(PDF_ENABLE_FREETYPE)
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  FT_FaceRec* rec = GetRec();
  if (!rec) {
    return nullptr;
  }
  FT_Matrix ft_matrix;
  ft_matrix.xx = matrix.a / 64 * 65536;
  ft_matrix.xy = matrix.c / 64 * 65536;
  ft_matrix.yx = matrix.b / 64 * 65536;
  ft_matrix.yy = matrix.d / 64 * 65536;
  if (subst_font) {
    int skew = subst_font->GetEffectiveSkew(is_cid_font);
    if (skew) {
      if (is_vertical) {
        ft_matrix.yx += ft_matrix.yy * skew / 100;
      } else {
        ft_matrix.xy -= ft_matrix.xx * skew / 100;
      }
    }
    if (subst_font->IsBuiltInGenericFont()) {
      AdjustVariationParams(glyph_index, dest_width, subst_font->GetWeight());
    }
  }

  int embolden_level = 0;
  if (subst_font) {
    embolden_level = subst_font->GetEmboldenLevelForRender(
        is_cid_font, static_cast<int32_t>(ft_matrix.xx),
        static_cast<int32_t>(ft_matrix.xy));
    if (embolden_level < 0) {
      return nullptr;
    }
  }

  auto* glyph = rec->glyph;
  glyph->format = FT_GLYPH_FORMAT_OUTLINE;

  ScopedFaceTransform scoped_transform(GetRec(), &ft_matrix);
  int load_flags = FT_LOAD_NO_BITMAP | FT_LOAD_PEDANTIC;
  if (!IsTtOt()) {
    load_flags |= FT_LOAD_NO_HINTING;
  }
  int error = FT_Load_Glyph(rec, glyph_index, load_flags);
  if (error) {
    if (load_flags & FT_LOAD_NO_HINTING) {
      return nullptr;
    }
    load_flags |= FT_LOAD_NO_HINTING;
    load_flags &= ~FT_LOAD_PEDANTIC;
    error = FT_Load_Glyph(rec, glyph_index, load_flags);
    if (error) {
      return nullptr;
    }
  }

  if (embolden_level > 0) {
    FT_Outline_Embolden(&glyph->outline, embolden_level);
  }
  CFX_FontMgr* font_mgr = CFX_GEModule::Get()->GetFontMgr();
  FT_Library_SetLcdFilter(font_mgr->GetFTLibrary(), FT_LCD_FILTER_DEFAULT);
  error =
      FT_Render_Glyph(glyph, FtRenderModeFromFontAntiAliasingMode(anti_alias));
  if (error) {
    return nullptr;
  }
  const FT_Bitmap& ft_bitmap = glyph->bitmap;
  if (ft_bitmap.width > kMaxGlyphDimension ||
      ft_bitmap.rows > kMaxGlyphDimension) {
    return nullptr;
  }
  int dib_width = ft_bitmap.width;
  const FXDIB_Format format = anti_alias == FontAntiAliasingMode::kMono
                                  ? FXDIB_Format::k1bppMask
                                  : FXDIB_Format::k8bppMask;
  RetainPtr<CFX_DIBitmap> new_bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!new_bitmap->Create(dib_width, ft_bitmap.rows, format)) {
    return nullptr;
  }
  auto glyph_bitmap = std::make_unique<CFX_GlyphBitmap>(
      CFX_Point(glyph->bitmap_left, glyph->bitmap_top), new_bitmap);

  const uint32_t src_pitch = abs(ft_bitmap.pitch);
  // SAFETY: `ft_bitmap.buffer` contains `src_pitch * ft_bitmap.rows` bytes
  // allocated and rendered by FreeType.
  pdfium::span<const uint8_t> src_span =
      UNSAFE_BUFFERS(pdfium::span<const uint8_t>(ft_bitmap.buffer,
                                                 src_pitch * ft_bitmap.rows));

  if (anti_alias != FontAntiAliasingMode::kMono &&
      ft_bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
    new_bitmap->Populate8bbpMaskFrom1bppSpan(src_span, src_pitch);
  } else {
    new_bitmap->PopulateFromSpan(src_span, src_pitch);
  }
  return glyph_bitmap;
#else
  return nullptr;
#endif  // defined(PDF_ENABLE_FREETYPE)
}

std::unique_ptr<CFX_Path> CFX_Face::LoadGlyphPath(
    uint32_t glyph_index,
    int dest_width,
    bool is_vertical,
    const CFX_SubstFont* subst_font) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (skrifa_font_ && skrifa_font_->font->is_ok()) {
      skrifa::Outline outline;
      if (skrifa_font_->font->unscaled_outline(glyph_index, outline)) {
        int upem = skrifa_font_->font->units_per_em();
        if (upem > 0) {
          float scale = 1.0f / static_cast<float>(upem);
          CFX_Matrix matrix(scale, 0, 0, scale, 0, 0);
          AdjustSubstFontTransform(subst_font, dest_width,
                                   outline.advance_width, /*is_cid_font=*/false,
                                   is_vertical, &matrix);
          auto path = ConvertOutline(outline);
          if (path) {
            path->Transform(matrix);
            return path;
          }
        }
      }
    }
    return nullptr;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return nullptr;
  }
  FT_FaceRec* rec = GetRec();
  FT_Set_Pixel_Sizes(rec, 0, 64);
  FT_Matrix ft_matrix = {65536, 0, 0, 65536};
  if (subst_font) {
    int skew = subst_font->GetSkew();
    if (skew) {
      if (is_vertical) {
        ft_matrix.yx += ft_matrix.yy * skew / 100;
      } else {
        ft_matrix.xy -= ft_matrix.xx * skew / 100;
      }
    }
    if (subst_font->IsBuiltInGenericFont()) {
      AdjustVariationParams(glyph_index, dest_width, subst_font->GetWeight());
    }
  }
  ScopedFaceTransform scoped_transform(GetRec(), &ft_matrix);
  int load_flags = FT_LOAD_NO_BITMAP;
  if (!IsTtOt() || !IsTricky()) {
    load_flags |= FT_LOAD_NO_HINTING;
  }
  if (FT_Load_Glyph(rec, glyph_index, load_flags)) {
    return nullptr;
  }
  if (subst_font) {
    int level = subst_font->GetEmboldenLevelForLoad();
    if (level > 0) {
      FT_Outline_Embolden(&rec->glyph->outline, level);
    }
  }

  FT_Outline_Funcs funcs;
  funcs.move_to = Outline_MoveTo;
  funcs.line_to = Outline_LineTo;
  funcs.conic_to = Outline_ConicTo;
  funcs.cubic_to = Outline_CubicTo;
  funcs.shift = 0;
  funcs.delta = 0;

  auto pPath = std::make_unique<CFX_Path>();
  OUTLINE_PARAMS params = {
      .path_ = pPath.get(),
      .cur_x_ = 0,
      .cur_y_ = 0,
  };

  FT_Outline_Decompose(&rec->glyph->outline, &funcs, &params);
  if (pPath->GetPoints().empty()) {
    return nullptr;
  }

  Outline_CheckEmptyContour(&params);
  pPath->ClosePath();

  return pPath;
#else
  return nullptr;
#endif  // defined(PDF_ENABLE_FREETYPE)
}

int CFX_Face::GetGlyphTTWidth(uint32_t glyph_index) const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    skrifa::Outline outline;
    if (skrifa_font_->font->unscaled_outline(glyph_index, outline)) {
      return NormalizeFontMetric(
          static_cast<int64_t>(outline.advance_width + 0.5), GetUnitsPerEm());
    }
    return 0;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  const auto* fontglyph = GetRec()->glyph;
  DCHECK_EQ(glyph_index, fontglyph->glyph_index);
  return NormalizeFontMetric(fontglyph->metrics.horiAdvance, GetUnitsPerEm());
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

int CFX_Face::GetGlyphWidth(uint32_t glyph_index,
                            int dest_width,
                            int weight,
                            const CFX_SubstFont* subst_font) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (subst_font && subst_font->IsBuiltInGenericFont() && dest_width > 0) {
      return dest_width;
    }
    skrifa::Outline outline;
    if (skrifa_font_->font->unscaled_outline(glyph_index, outline)) {
      return EmAdjust(static_cast<int>(outline.advance_width));
    }
    return 0;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (subst_font && subst_font->IsBuiltInGenericFont()) {
    AdjustVariationParams(glyph_index, dest_width, weight);
  }

  FT_FaceRec* rec = GetRec();
  if (!rec) {
    return 0;
  }
  int err = FT_Load_Glyph(
      rec, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
  if (err) {
    return 0;
  }

  FT_Pos horizontal_advance = rec->glyph->metrics.horiAdvance;
  if (horizontal_advance < kThousandthMinInt ||
      horizontal_advance > kThousandthMaxInt) {
    return 0;
  }

  return EmAdjust(static_cast<int>(horizontal_advance));
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

ByteString CFX_Face::GetGlyphName(uint32_t glyph_index) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    rust::String skrifa_result = skrifa_font_->font->glyph_name(glyph_index);
    return ByteString(skrifa_result.c_str());
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return ByteString();
  }
  char name[256] = {};
  FT_Get_Glyph_Name(GetRec(), glyph_index, name, sizeof(name));
  name[255] = 0;
  return ByteString(name);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

int CFX_Face::GetCharIndex(uint32_t code) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    const bool unicode_or_none =
        selected_encoding_ == fxge::FontEncoding::kUnicode ||
        selected_encoding_ == fxge::FontEncoding::kNone;
    if (IsPostScriptFont()) {
      if (unicode_or_none) {
        return static_cast<int>(skrifa_font_->font->unicode_to_gid(code));
      }
      if (code <= 0xFF) {
        return static_cast<int>(
            skrifa_font_->font->code_to_gid(static_cast<uint8_t>(code)));
      }
      return 0;
    }
    if (selected_charmap_index_.has_value() && !unicode_or_none) {
      uint32_t gid = skrifa_font_->font->cmap_char_to_gid(
          selected_charmap_index_.value(), code);
      if (gid != 0) {
        return static_cast<int>(gid);
      }
    }
    if (unicode_or_none) {
      uint32_t gid = skrifa_font_->font->unicode_to_gid(code);
      if (gid != 0) {
        return static_cast<int>(gid);
      }
    }
    if (selected_charmap_index_.has_value()) {
      return static_cast<int>(skrifa_font_->font->cmap_char_to_gid(
          selected_charmap_index_.value(), code));
    }
    return 0;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return 0;
  }
  return FT_Get_Char_Index(GetRec(), code);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

int CFX_Face::GetNameIndex(const char* name) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return static_cast<int>(skrifa_font_->font->name_index(name));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return 0;
  }
  return FT_Get_Name_Index(GetRec(), name);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

int CFX_Face::LoadGlyph(uint32_t glyph_index, bool scale) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return skrifa_font_->font->has_outline(glyph_index) ? 0 : -1;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return -1;
  }
  FT_Int32 args = FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH;
  if (!scale) {
    args |= FT_LOAD_NO_SCALE;
  }
  return FT_Load_Glyph(GetRec(), glyph_index, args);
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

ByteString CFX_Face::GetPostscriptName() {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    rust::Str skrifa_result = skrifa_font_->font->postscript_name();
    return ByteString(ByteStringView(skrifa_result));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return ByteString();
  }
  return ByteString(FT_Get_Postscript_Name(GetRec()));
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

CFX_Size CFX_Face::GetPixelSize() const {
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return {64, 64};
  }
  int pixel_size_x = GetRec()->size->metrics.x_ppem;
  int pixel_size_y = GetRec()->size->metrics.y_ppem;
  return {pixel_size_x, pixel_size_y};
#else
  return {64, 64};
#endif  // defined(PDF_ENABLE_FREETYPE)
}

std::optional<FX_RECT> CFX_Face::GetFontGlyphBBox(uint32_t glyph_index) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (!skrifa_font_->font->has_outline(glyph_index)) {
      return std::nullopt;
    }
    skrifa::BoundingBox bbox = skrifa_font_->font->glyph_bounds(glyph_index);
    const uint16_t upem = GetUnitsPerEm();
    return FX_RECT(NormalizeFontMetric(bbox.x_min, upem),
                   NormalizeFontMetric(bbox.y_min, upem),
                   NormalizeFontMetric(bbox.x_max, upem),
                   NormalizeFontMetric(bbox.y_max, upem));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (IsTricky()) {
    int error = FT_Set_Char_Size(GetRec(), 0, 1000 * 64, 72, 72);
    if (error) {
      return std::nullopt;
    }

    error = LoadGlyph(glyph_index, /*scale=*/true);
    if (error) {
      return std::nullopt;
    }

    FT_Glyph glyph;
    error = FT_Get_Glyph(GetRec()->glyph, &glyph);
    if (error) {
      return std::nullopt;
    }

    FT_BBox cbox;
    FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &cbox);
    CFX_Size pixelSize = GetPixelSize();
    FX_RECT result =
        ScaledFXRectFromFTPos(cbox.xMin, cbox.yMax, cbox.xMax, cbox.yMin,
                              pixelSize.width, pixelSize.height);
    result.top = std::min(result.top, static_cast<int>(GetAscender()));
    result.bottom = std::max(result.bottom, static_cast<int>(GetDescender()));
    FT_Done_Glyph(glyph);
    return result;
  }
  if (LoadGlyph(glyph_index, /*scale=*/false) != 0) {
    return std::nullopt;
  }
  int em = GetUnitsPerEm();
  return ScaledFXRectFromFTPos(
      GetRec()->glyph->metrics.horiBearingX,
      GetRec()->glyph->metrics.horiBearingY - GetRec()->glyph->metrics.height,
      GetRec()->glyph->metrics.horiBearingX + GetRec()->glyph->metrics.width,
      GetRec()->glyph->metrics.horiBearingY, em, em);
#else
  return std::nullopt;
#endif  // defined(PDF_ENABLE_FREETYPE)
}

FX_RECT CFX_Face::GetCharBBox(uint32_t code, int glyph_index) {
  FX_RECT rect;
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (glyph_index < 0 || LoadGlyph(glyph_index, /*scale=*/false) != 0) {
      return rect;
    }
    rect = GetGlyphBBox(glyph_index);
    if (IsTricky()) {
      rect.top = std::min(rect.top, static_cast<int>(GetAscender()));
      rect.bottom = std::max(rect.bottom, static_cast<int>(GetDescender()));
    } else {
      if (rect.top <= kMaxRectTop) {
        rect.top += rect.top / 64;
      } else {
        rect.top = std::numeric_limits<int>::max();
      }
    }
    return rect;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  FT_FaceRec* rec = GetRec();
  if (!rec) {
    return rect;
  }
  if (IsTricky()) {
    int err =
        FT_Load_Glyph(rec, glyph_index, FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    if (!err) {
      FT_Glyph glyph;
      err = FT_Get_Glyph(rec->glyph, &glyph);
      if (!err) {
        FT_BBox cbox;
        FT_Glyph_Get_CBox(glyph, FT_GLYPH_BBOX_PIXELS, &cbox);
        const int xMin = FTPosToCBoxInt(cbox.xMin);
        const int xMax = FTPosToCBoxInt(cbox.xMax);
        const int yMin = FTPosToCBoxInt(cbox.yMin);
        const int yMax = FTPosToCBoxInt(cbox.yMax);
        const int pixel_size_x = rec->size->metrics.x_ppem;
        const int pixel_size_y = rec->size->metrics.y_ppem;
        if (pixel_size_x == 0 || pixel_size_y == 0) {
          rect = FX_RECT(xMin, yMax, xMax, yMin);
        } else {
          rect =
              FX_RECT(xMin * 1000 / pixel_size_x, yMax * 1000 / pixel_size_y,
                      xMax * 1000 / pixel_size_x, yMin * 1000 / pixel_size_y);
        }
        rect.top = std::min(rect.top, static_cast<int>(GetAscender()));
        rect.bottom = std::max(rect.bottom, static_cast<int>(GetDescender()));
        FT_Done_Glyph(glyph);
      }
    }
  } else {
    int err = LoadGlyph(glyph_index, /*scale=*/false);
    if (err == 0) {
      rect = GetGlyphBBox(glyph_index);
      if (rect.top <= kMaxRectTop) {
        rect.top += rect.top / 64;
      } else {
        rect.top = std::numeric_limits<int>::max();
      }
    }
  }
#endif  // defined(PDF_ENABLE_FREETYPE)
  return rect;
}

FX_RECT CFX_Face::GetGlyphBBox(uint32_t glyph_index) const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    skrifa::BoundingBox bbox = skrifa_font_->font->glyph_bounds(glyph_index);
    const uint16_t upem = GetUnitsPerEm();
    return FX_RECT(NormalizeFontMetric(bbox.x_min, upem),
                   NormalizeFontMetric(bbox.y_max, upem),
                   NormalizeFontMetric(bbox.x_max, upem),
                   NormalizeFontMetric(bbox.y_min, upem));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  const auto* glyph = GetRec()->glyph;
  DCHECK_EQ(glyph_index, glyph->glyph_index);

  pdfium::ClampedNumeric<FT_Pos> left = glyph->metrics.horiBearingX;
  pdfium::ClampedNumeric<FT_Pos> top = glyph->metrics.horiBearingY;
  const uint16_t upem = GetUnitsPerEm();
  return FX_RECT(NormalizeFontMetric(left, upem),
                 NormalizeFontMetric(top, upem),
                 NormalizeFontMetric(left + glyph->metrics.width, upem),
                 NormalizeFontMetric(top - glyph->metrics.height, upem));
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

std::vector<CharCodeAndIndex> CFX_Face::GetCharCodesAndIndices(
    char32_t max_char) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    auto skrifa_result =
        skrifa_font_->font->get_char_codes_and_indices(max_char);
    std::vector<CharCodeAndIndex> results;
    results.reserve(skrifa_result.size());
    for (const auto& item : skrifa_result) {
      results.push_back({item.char_code, item.glyph_index});
    }
    return results;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  CharCodeAndIndex char_code_and_index;
  char_code_and_index.char_code = static_cast<uint32_t>(
      FT_Get_First_Char(GetRec(), &char_code_and_index.glyph_index));
  if (char_code_and_index.char_code > max_char) {
    return {};
  }
  std::vector<CharCodeAndIndex> results = {char_code_and_index};
  while (true) {
    char_code_and_index.char_code = static_cast<uint32_t>(FT_Get_Next_Char(
        GetRec(), results.back().char_code, &char_code_and_index.glyph_index));
    if (char_code_and_index.char_code > max_char ||
        char_code_and_index.glyph_index == 0) {
      break;
    }
    results.push_back(char_code_and_index);
  }
  return results;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

CFX_Face::CharMap CFX_Face::GetCurrentCharMap() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (selected_charmap_index_.has_value()) {
      return reinterpret_cast<CharMap>(selected_charmap_index_.value() + 1);
    }
    return nullptr;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return GetRec() ? GetRec()->charmap : nullptr;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

std::optional<fxge::FontEncoding> CFX_Face::GetCurrentCharMapEncoding() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (selected_encoding_ != fxge::FontEncoding::kNone) {
      return selected_encoding_;
    }
    if (selected_charmap_index_.has_value()) {
      return GetCharMapEncodingByIndex(selected_charmap_index_.value());
    }
    return fxge::FontEncoding::kUnicode;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec() || !GetRec()->charmap) {
    return std::nullopt;
  }
  return CharMapIdPairToFontEncoding(
      {.platform_id = GetRec()->charmap->platform_id,
       .encoding_id = GetRec()->charmap->encoding_id});
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

CFX_Face::CharMapIdPair CFX_Face::GetCharMapIdPairByIndex(size_t index) const {
  return {.platform_id = GetCharMapPlatformIdByIndex(index),
          .encoding_id = GetCharMapEncodingIdByIndex(index)};
}

uint16_t CFX_Face::GetCharMapPlatformIdByIndex(size_t index) const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (IsPostScriptFont()) {
      return index == 0 ? kPlatformAppleUnicode : kPlatformAdobe;
    }
    skrifa::CharMapInfo info;
    if (skrifa_font_->font->get_charmap_info(index, info)) {
      return info.platform_id;
    }
    return 0;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return 0;
  }
  return GetCharMaps()[index]->platform_id;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

uint16_t CFX_Face::GetCharMapEncodingIdByIndex(size_t index) const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (IsPostScriptFont()) {
      return index == 0 ? kAppleUnicodeEncodingUnicode2_0
                        : kAdobeEncodingCustom;
    }
    skrifa::CharMapInfo info;
    if (skrifa_font_->font->get_charmap_info(index, info)) {
      return info.encoding_id;
    }
    return 0;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return 0;
  }
  return GetCharMaps()[index]->encoding_id;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

fxge::FontEncoding CFX_Face::GetCharMapEncodingByIndex(size_t index) const {
  return CharMapIdPairToFontEncoding(GetCharMapIdPairByIndex(index));
}

size_t CFX_Face::GetCharMapCount() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (IsPostScriptFont()) {
      return skrifa_font_->font->encoding() != skrifa::PsEncodingKind::None ? 2
                                                                            : 1;
    }
    return skrifa_font_->font->get_charmap_count();
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return GetRec() && GetRec()->charmaps
             ? pdfium::checked_cast<size_t>(GetRec()->num_charmaps)
             : 0;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

#if defined(PDF_ENABLE_FREETYPE)
pdfium::span<const FT_CharMap> CFX_Face::GetCharMaps() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    return {};
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
  if (!GetRec()) {
    return {};
  }
  size_t count = GetRec()->charmaps
                     ? pdfium::checked_cast<size_t>(GetRec()->num_charmaps)
                     : 0;
  if (count == 0) {
    return {};
  }
  // SAFETY: required from library to provide correct count.
  return UNSAFE_BUFFERS({GetRec()->charmaps, count});
}
#endif  // defined(PDF_ENABLE_FREETYPE)

void CFX_Face::SetCharMap(CharMap map) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (map) {
      size_t index = reinterpret_cast<uintptr_t>(map) - 1;
      if (index < GetCharMapCount()) {
        SetCharMapByIndex(index);
        return;
      }
    }
    selected_charmap_index_ = std::nullopt;
    selected_encoding_ = fxge::FontEncoding::kNone;
    return;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (GetRec()) {
    FT_Set_Charmap(GetRec(), static_cast<FT_CharMap>(map));
  }
#endif  // defined(PDF_ENABLE_FREETYPE)
}

void CFX_Face::SetCharMapByIndex(size_t index) {
  CHECK_LT(index, GetCharMapCount());

#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    selected_charmap_index_ = index;
    selected_encoding_ = GetCharMapEncodingByIndex(index);
    return;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)

#if defined(PDF_ENABLE_FREETYPE)
  // SAFETY: required from library as enforced by check above.
  SetCharMap(UNSAFE_BUFFERS(GetRec()->charmaps[index]));
#endif  // defined(PDF_ENABLE_FREETYPE)
}

bool CFX_Face::SelectCharMap(fxge::FontEncoding encoding) {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    if (IsPostScriptFont()) {
      if (encoding == fxge::FontEncoding::kUnicode) {
        selected_encoding_ = encoding;
        selected_charmap_index_ = 0;
        return true;
      }
      if (skrifa_font_->font->encoding() != skrifa::PsEncodingKind::None) {
        selected_encoding_ = encoding;
        selected_charmap_index_ = 1;
        return true;
      }
      return false;
    }
    for (size_t i = 0; i < GetCharMapCount(); ++i) {
      if (GetCharMapEncodingByIndex(i) == encoding) {
        selected_encoding_ = encoding;
        selected_charmap_index_ = i;
        return true;
      }
    }
    return false;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return false;
  }
  FT_Error error = FT_Select_Charmap(GetRec(), ToFTEncoding(encoding));
  if (error) {
    return false;
  }
  selected_encoding_ = encoding;
  return true;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}

#if defined(PDF_ENABLE_XFA)
int CFX_Face::GetNumFaces() const {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    pdfium::span<const uint8_t> data = GetData();
    return static_cast<int>(
        skrifa::get_num_faces(rust::Slice<const uint8_t>(data)));
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  return GetRec() ? pdfium::checked_cast<int>(GetRec()->num_faces) : 1;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}
#endif  // defined(PDF_ENABLE_XFA)

#if BUILDFLAG(IS_WIN)
bool CFX_Face::CanEmbed() {
#if defined(PDF_ENABLE_FONTATIONS)
  if (CFX_GEModule::IsFontations()) {
    uint16_t fs_type = 0;
    if (skrifa_font_->font->get_os2_fs_type(fs_type)) {
      return (fs_type &
              (fxcrt::to_underlying(
                   skrifa::FsType::RestrictedLicenseEmbedding) |
               fxcrt::to_underlying(skrifa::FsType::BitmapEmbeddingOnly))) == 0;
    }
    return true;
  }
#endif  // defined(PDF_ENABLE_FONTATIONS)
#if defined(PDF_ENABLE_FREETYPE)
  if (!GetRec()) {
    return true;
  }
  FT_UShort fstype = FT_Get_FSType_Flags(GetRec());
  return (fstype & (FT_FSTYPE_RESTRICTED_LICENSE_EMBEDDING |
                    FT_FSTYPE_BITMAP_EMBEDDING_ONLY)) == 0;
#endif  // defined(PDF_ENABLE_FREETYPE)
  NOTREACHED();
}
#endif  // BUILDFLAG(IS_WIN)

CFX_Face::CFX_Face(RetainPtr<Retainable> cache_entry,
                   RetainPtr<CFX_ReadOnlySpanStream> font_stream,
                   [[maybe_unused]] FT_FaceRec* rec,
                   [[maybe_unused]] SkrifaFontHolder* skrifa_font)
    : cache_entry_(std::move(cache_entry)),
      font_stream_(std::move(font_stream))
#if defined(PDF_ENABLE_FREETYPE)
      ,
      rec_(rec)
#endif
#if defined(PDF_ENABLE_FONTATIONS)
      ,
      skrifa_font_(skrifa_font)
#endif  // defined(PDF_ENABLE_FONTATIONS)
{
#if defined(PDF_ENABLE_FREETYPE) && defined(PDF_ENABLE_FONTATIONS)
  DCHECK(rec_ || skrifa_font_);
#elif defined(PDF_ENABLE_FREETYPE)
  DCHECK(rec_);
#elif defined(PDF_ENABLE_FONTATIONS)
  DCHECK(skrifa_font_);
#endif
}

#if defined(PDF_USE_SKIA)
SkTypeface* CFX_Face::GetOrCreateSkTypeface() {
  if (!skia_typeface_) {
    skia_typeface_ =
        CFX_GEModule::Get()->GetFontMgr()->MakeSkTypeface(GetData());
  }
  return skia_typeface_.get();
}
#endif  // defined(PDF_USE_SKIA)

CFX_Face::~CFX_Face() = default;

#if defined(PDF_ENABLE_FREETYPE)
void CFX_Face::AdjustVariationParams(int glyph_index,
                                     int dest_width,
                                     int weight) {
  // TODO(https://crbug.com/42271123): Implement variation parameters adjustment
  // in Skia/Fontations.
  DCHECK_GE(dest_width, 0);

  FT_FaceRec* rec = GetRec();
  if (!rec) {
    return;
  }
  ScopedFXFTMMVar variation_desc(rec);
  if (!variation_desc) {
    return;
  }

  FT_Pos coords[2];
  if (weight == 0) {
    coords[0] = variation_desc.GetAxisDefault(0) / 65536;
  } else {
    coords[0] = weight;
  }

  if (dest_width == 0) {
    coords[1] = variation_desc.GetAxisDefault(1) / 65536;
  } else {
    FT_Long min_param = variation_desc.GetAxisMin(1) / 65536;
    FT_Long max_param = variation_desc.GetAxisMax(1) / 65536;
    coords[1] = min_param;
    FT_Set_MM_Design_Coordinates(rec, 2, coords);
    FT_Load_Glyph(rec, glyph_index,
                  FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    FT_Pos min_width = rec->glyph->metrics.horiAdvance * 1000 / GetUnitsPerEm();
    coords[1] = max_param;
    FT_Set_MM_Design_Coordinates(rec, 2, coords);
    FT_Load_Glyph(rec, glyph_index,
                  FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH);
    FT_Pos max_width = rec->glyph->metrics.horiAdvance * 1000 / GetUnitsPerEm();
    if (max_width == min_width) {
      return;
    }
    FT_Pos param = min_param + (max_param - min_param) *
                                   (dest_width - min_width) /
                                   (max_width - min_width);
    coords[1] = param;
  }
  FT_Set_MM_Design_Coordinates(rec, 2, coords);
}
#endif  // defined(PDF_ENABLE_FREETYPE)

#if defined(PDF_ENABLE_FONTATIONS)
void CFX_Face::AdjustSubstFontTransform(const CFX_SubstFont* subst_font,
                                        int dest_width,
                                        float advance_width,
                                        bool is_cid_font,
                                        bool is_vertical,
                                        CFX_Matrix* matrix) const {
  if (!subst_font) {
    return;
  }
  if (subst_font->IsBuiltInGenericFont() && dest_width > 0 &&
      advance_width > 0) {
    int glyph_width = EmAdjust(static_cast<int>(advance_width));
    if (glyph_width > 0) {
      float scale_x = static_cast<float>(dest_width) / glyph_width;
      matrix->a *= scale_x;
      matrix->b *= scale_x;
    }
  }
  int skew = subst_font->GetEffectiveSkew(is_cid_font);
  if (skew) {
    if (is_vertical) {
      matrix->b += matrix->d * skew / 100.0f;
    } else {
      matrix->c -= matrix->a * skew / 100.0f;
    }
  }
}
#endif  // defined(PDF_ENABLE_FONTATIONS)

#if defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_LINUX)
uint32_t CFX_Face::GetFontStyle() {
  uint32_t style = 0;
  if (IsBold()) {
    style |= pdfium::kFontStyleForceBold;
  }
  if (IsItalic()) {
    style |= pdfium::kFontStyleItalic;
  }
  if (IsFixedWidth()) {
    style |= pdfium::kFontStyleFixedPitch;
  }

  std::optional<std::array<uint32_t, 2>> code_page_range =
      GetOs2CodePageRange();
  if (code_page_range.has_value() && (code_page_range.value()[0] & (1 << 31))) {
    style |= pdfium::kFontStyleSymbolic;
  }

  std::optional<std::array<uint8_t, 2>> panose = GetOs2Panose();
  if (panose.has_value() && panose.value()[0] == 2) {
    uint8_t serif = panose.value()[1];
    if ((serif > 1 && serif < 10) || serif > 13) {
      style |= pdfium::kFontStyleSerif;
    }
  }
  return style;
}
#endif  // defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID) ||
        // BUILDFLAG(IS_LINUX)
