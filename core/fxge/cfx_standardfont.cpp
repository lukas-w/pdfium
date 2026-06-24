// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_standardfont.h"

#include <algorithm>
#include <array>
#include <iterator>

#include "core/fxcrt/containers/contains.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxge/fontdata/chromefontdata/chromefontdata.h"

namespace {

static_assert(CFX_StandardFont::kLast + 1 ==
                  CFX_StandardFont::kNumStandardFonts,
              "StandardFont enum count mismatch");

constexpr std::array<pdfium::span<const uint8_t>,
                     CFX_StandardFont::kNumStandardFonts>
    kFoxitFonts = {{
        kFoxitFixedFontData,
        kFoxitFixedBoldFontData,
        kFoxitFixedBoldItalicFontData,
        kFoxitFixedItalicFontData,
        kFoxitSansFontData,
        kFoxitSansBoldFontData,
        kFoxitSansBoldItalicFontData,
        kFoxitSansItalicFontData,
        kFoxitSerifFontData,
        kFoxitSerifBoldFontData,
        kFoxitSerifBoldItalicFontData,
        kFoxitSerifItalicFontData,
        kFoxitSymbolFontData,
        kFoxitDingbatsFontData,
    }};

constexpr pdfium::span<const uint8_t> kGenericSansFont = kFoxitSansMMFontData;
constexpr pdfium::span<const uint8_t> kGenericSerifFont = kFoxitSerifMMFontData;

constexpr std::array<const char*, CFX_StandardFont::kNumStandardFonts>
    kBase14FontNames = {{
        "Courier",
        "Courier-Bold",
        "Courier-BoldOblique",
        "Courier-Oblique",
        "Helvetica",
        "Helvetica-Bold",
        "Helvetica-BoldOblique",
        "Helvetica-Oblique",
        "Times-Roman",
        "Times-Bold",
        "Times-BoldItalic",
        "Times-Italic",
        "Symbol",
        "ZapfDingbats",
    }};

struct AltFontName {
  const char* name_;  // Raw, POD struct.
  CFX_StandardFont::StandardFont index_;
};

constexpr AltFontName kAltFontNames[] = {
    {"Arial", CFX_StandardFont::kHelvetica},
    {"Arial,Bold", CFX_StandardFont::kHelveticaBold},
    {"Arial,BoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"Arial,Italic", CFX_StandardFont::kHelveticaOblique},
    {"Arial-Bold", CFX_StandardFont::kHelveticaBold},
    {"Arial-BoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"Arial-BoldItalicMT", CFX_StandardFont::kHelveticaBoldOblique},
    {"Arial-BoldMT", CFX_StandardFont::kHelveticaBold},
    {"Arial-Italic", CFX_StandardFont::kHelveticaOblique},
    {"Arial-ItalicMT", CFX_StandardFont::kHelveticaOblique},
    {"ArialBold", CFX_StandardFont::kHelveticaBold},
    {"ArialBoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"ArialItalic", CFX_StandardFont::kHelveticaOblique},
    {"ArialMT", CFX_StandardFont::kHelvetica},
    {"ArialMT,Bold", CFX_StandardFont::kHelveticaBold},
    {"ArialMT,BoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"ArialMT,Italic", CFX_StandardFont::kHelveticaOblique},
    {"ArialRoundedMTBold", CFX_StandardFont::kHelveticaBold},
    {"Courier", CFX_StandardFont::kCourier},
    {"Courier,Bold", CFX_StandardFont::kCourierBold},
    {"Courier,BoldItalic", CFX_StandardFont::kCourierBoldOblique},
    {"Courier,Italic", CFX_StandardFont::kCourierOblique},
    {"Courier-Bold", CFX_StandardFont::kCourierBold},
    {"Courier-BoldOblique", CFX_StandardFont::kCourierBoldOblique},
    {"Courier-Oblique", CFX_StandardFont::kCourierOblique},
    {"CourierBold", CFX_StandardFont::kCourierBold},
    {"CourierBoldItalic", CFX_StandardFont::kCourierBoldOblique},
    {"CourierItalic", CFX_StandardFont::kCourierOblique},
    {"CourierNew", CFX_StandardFont::kCourier},
    {"CourierNew,Bold", CFX_StandardFont::kCourierBold},
    {"CourierNew,BoldItalic", CFX_StandardFont::kCourierBoldOblique},
    {"CourierNew,Italic", CFX_StandardFont::kCourierOblique},
    {"CourierNew-Bold", CFX_StandardFont::kCourierBold},
    {"CourierNew-BoldItalic", CFX_StandardFont::kCourierBoldOblique},
    {"CourierNew-Italic", CFX_StandardFont::kCourierOblique},
    {"CourierNewBold", CFX_StandardFont::kCourierBold},
    {"CourierNewBoldItalic", CFX_StandardFont::kCourierBoldOblique},
    {"CourierNewItalic", CFX_StandardFont::kCourierOblique},
    {"CourierNewPS-BoldItalicMT", CFX_StandardFont::kCourierBoldOblique},
    {"CourierNewPS-BoldMT", CFX_StandardFont::kCourierBold},
    {"CourierNewPS-ItalicMT", CFX_StandardFont::kCourierOblique},
    {"CourierNewPSMT", CFX_StandardFont::kCourier},
    {"CourierStd", CFX_StandardFont::kCourier},
    {"CourierStd-Bold", CFX_StandardFont::kCourierBold},
    {"CourierStd-BoldOblique", CFX_StandardFont::kCourierBoldOblique},
    {"CourierStd-Oblique", CFX_StandardFont::kCourierOblique},
    {"Helvetica", CFX_StandardFont::kHelvetica},
    {"Helvetica,Bold", CFX_StandardFont::kHelveticaBold},
    {"Helvetica,BoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"Helvetica,Italic", CFX_StandardFont::kHelveticaOblique},
    {"Helvetica-Bold", CFX_StandardFont::kHelveticaBold},
    {"Helvetica-BoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"Helvetica-BoldOblique", CFX_StandardFont::kHelveticaBoldOblique},
    {"Helvetica-Italic", CFX_StandardFont::kHelveticaOblique},
    {"Helvetica-Oblique", CFX_StandardFont::kHelveticaOblique},
    {"HelveticaBold", CFX_StandardFont::kHelveticaBold},
    {"HelveticaBoldItalic", CFX_StandardFont::kHelveticaBoldOblique},
    {"HelveticaItalic", CFX_StandardFont::kHelveticaOblique},
    {"Symbol", CFX_StandardFont::kSymbol},
    {"SymbolMT", CFX_StandardFont::kSymbol},
    {"Times-Bold", CFX_StandardFont::kTimesBold},
    {"Times-BoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"Times-Italic", CFX_StandardFont::kTimesOblique},
    {"Times-Roman", CFX_StandardFont::kTimes},
    {"TimesBold", CFX_StandardFont::kTimesBold},
    {"TimesBoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"TimesItalic", CFX_StandardFont::kTimesOblique},
    {"TimesNewRoman", CFX_StandardFont::kTimes},
    {"TimesNewRoman,Bold", CFX_StandardFont::kTimesBold},
    {"TimesNewRoman,BoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"TimesNewRoman,Italic", CFX_StandardFont::kTimesOblique},
    {"TimesNewRoman-Bold", CFX_StandardFont::kTimesBold},
    {"TimesNewRoman-BoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"TimesNewRoman-Italic", CFX_StandardFont::kTimesOblique},
    {"TimesNewRomanBold", CFX_StandardFont::kTimesBold},
    {"TimesNewRomanBoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"TimesNewRomanItalic", CFX_StandardFont::kTimesOblique},
    {"TimesNewRomanPS", CFX_StandardFont::kTimes},
    {"TimesNewRomanPS-Bold", CFX_StandardFont::kTimesBold},
    {"TimesNewRomanPS-BoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"TimesNewRomanPS-BoldItalicMT", CFX_StandardFont::kTimesBoldOblique},
    {"TimesNewRomanPS-BoldMT", CFX_StandardFont::kTimesBold},
    {"TimesNewRomanPS-Italic", CFX_StandardFont::kTimesOblique},
    {"TimesNewRomanPS-ItalicMT", CFX_StandardFont::kTimesOblique},
    {"TimesNewRomanPSMT", CFX_StandardFont::kTimes},
    {"TimesNewRomanPSMT,Bold", CFX_StandardFont::kTimesBold},
    {"TimesNewRomanPSMT,BoldItalic", CFX_StandardFont::kTimesBoldOblique},
    {"TimesNewRomanPSMT,Italic", CFX_StandardFont::kTimesOblique},
    {"ZapfDingbats", CFX_StandardFont::kDingbats},
};

}  // namespace

// static
std::optional<CFX_StandardFont::StandardFont>
CFX_StandardFont::GetStandardFontName(ByteString* name) {
  const auto* end = std::end(kAltFontNames);
  const auto* found =
      std::lower_bound(std::begin(kAltFontNames), end, name->c_str(),
                       [](const AltFontName& element, const char* name) {
                         return FXSYS_stricmp(element.name_, name) < 0;
                       });
  if (found == end || FXSYS_stricmp(found->name_, name->c_str())) {
    return std::nullopt;
  }

  *name = kBase14FontNames[static_cast<size_t>(found->index_)];
  return found->index_;
}

// static
ByteString CFX_StandardFont::GetCanonicalFontName(StandardFont font) {
  return kBase14FontNames[static_cast<size_t>(font)];
}

// static
bool CFX_StandardFont::IsStandardFontName(const ByteString& name) {
  return pdfium::Contains(kBase14FontNames, name);
}

// static
bool CFX_StandardFont::IsSymbolicFont(StandardFont font) {
  return font == StandardFont::kSymbol || font == StandardFont::kDingbats;
}

// static
bool CFX_StandardFont::IsFixedFont(StandardFont font) {
  return font == StandardFont::kCourier || font == StandardFont::kCourierBold ||
         font == StandardFont::kCourierBoldOblique ||
         font == StandardFont::kCourierOblique;
}

// static
pdfium::span<const uint8_t> CFX_StandardFont::GetStandardFont(size_t index) {
  return kFoxitFonts[index];
}

// static
pdfium::span<const uint8_t> CFX_StandardFont::GetGenericSansFont() {
  return kGenericSansFont;
}

// static
pdfium::span<const uint8_t> CFX_StandardFont::GetGenericSerifFont() {
  return kGenericSerifFont;
}
