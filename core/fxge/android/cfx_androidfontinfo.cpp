// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/android/cfx_androidfontinfo.h"

#include <algorithm>
#include <functional>

#include "core/fxcrt/compiler_specific.h"
#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxge/android/cfpf_skiafont.h"
#include "core/fxge/android/cfpf_skiafontmgr.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/fx_font.h"

namespace {

constexpr char kRoboto[] = "Roboto";
constexpr char kDroidSans[] = "Droid Sans";
constexpr char kDroidSerif[] = "Droid Serif";
constexpr char kDroidSansMono[] = "Droid Sans Mono";
constexpr char kDroidSansFallback[] = "Droid Sans Fallback";

// Repeat the function from bytestring.cpp here, so it can be used in
// a constexpr context. Then make the compiler prove the mappings are
// correct.
constexpr uint32_t HashNormalizeFontName(const char* family) {
  uint32_t hash_code = 0;
  UNSAFE_BUFFERS({
    for (size_t i = 0; family[i] != '\0'; ++i) {
      char ch = family[i];
      if (ch == ' ' || ch == '-' || ch == ',') {
        continue;
      }
      hash_code = 31 * hash_code + FXSYS_ToLowerASCII(ch);
    }
  });
  return hash_code;
}

static_assert(HashNormalizeFontName("Arial") == 0x058c5083);
static_assert(HashNormalizeFontName("Baskerville") == 0x3d49f40e);
static_assert(HashNormalizeFontName("Courier New") == 0x83746053);
static_assert(HashNormalizeFontName("Courier") == 0x3918fe2d);
static_assert(HashNormalizeFontName("Cursive") == 0x432c41c5);
static_assert(HashNormalizeFontName("Fantasy") == 0xbf85ff26);
static_assert(HashNormalizeFontName("Georgia") == 0xfb4ce0de);
static_assert(HashNormalizeFontName("Goudy") == 0x05dfade2);
static_assert(HashNormalizeFontName("Monaco") == 0xc04fe601);
static_assert(HashNormalizeFontName("Monospace") == 0xaaa60c03);
static_assert(HashNormalizeFontName("Palatino") == 0x3b98b31c);
static_assert(HashNormalizeFontName("STSong") == 0xcad5eaf6);
static_assert(HashNormalizeFontName("Sanserif") == 0x779ce19d);
static_assert(HashNormalizeFontName("Serif") == 0x0684317d);
static_assert(HashNormalizeFontName("SimHei") == 0xca3812d5);
static_assert(HashNormalizeFontName("SimSun") == 0xca383e15);
static_assert(HashNormalizeFontName("Tahoma") == 0xcb7a04c8);
static_assert(HashNormalizeFontName("Verdana") == 0x14ee2d13);

const SkiaFontMap kSkiaFontmap[] = {
    {0x058c5083, kRoboto},             // Arial
    {0x05dfade2, kDroidSerif},         // Goudy
    {0x0684317d, kDroidSerif},         // Serif
    {0x14ee2d13, kRoboto},             // Verdana
    {0x3918fe2d, kDroidSansMono},      // Courier
    {0x3b98b31c, kDroidSerif},         // Palatino
    {0x3d49f40e, kDroidSerif},         // Baskerville
    {0x432c41c5, kDroidSerif},         // Cursive
    {0x491b6ad0, kDroidSerif},         // Unknown
    {0x5612cab1, kDroidSansFallback},  // Unknown
    {0x779ce19d, kRoboto},             // Sanserif
    {0x7cc9510b, kDroidSansFallback},  // Unknown
    {0x83746053, kDroidSansMono},      // Courier New
    {0xaaa60c03, kDroidSansMono},      // Monospace
    {0xbf85ff26, kDroidSerif},         // Fantasy
    {0xc04fe601, kDroidSansMono},      // Monaco
    {0xca3812d5, kDroidSansFallback},  // SimHei
    {0xca383e15, kDroidSansFallback},  // SimSun
    {0xcad5eaf6, kDroidSansFallback},  // STSong
    {0xcb7a04c8, kRoboto},             // Tahoma
    {0xfb4ce0de, kDroidSerif},         // Georgia
};

const SkiaFontMap kSkiaSansFontMap[] = {
    {0x058c5083, kDroidSans},  // Arial
    {0x14ee2d13, kDroidSans},  // Verdana
    {0x779ce19d, kDroidSans},  // Sanserif
    {0xcb7a04c8, kDroidSans},  // Tahoma
    {0xfb4ce0de, kDroidSans},  // Georgia
};

}  // namespace

pdfium::span<const SkiaFontMap> GetSkiaFontmap() {
  return kSkiaFontmap;
}

pdfium::span<const SkiaFontMap> GetSkiaSansFontMap() {
  return kSkiaSansFontMap;
}

uint32_t SkiaNormalizeFontName(ByteStringView family) {
  uint32_t hash_code = 0;
  for (char ch : family) {
    if (ch == ' ' || ch == '-' || ch == ',') {
      continue;
    }
    hash_code = 31 * hash_code + FXSYS_ToLowerASCII(ch);
  }
  return hash_code;
}

const char* SkiaGetSubstFont(uint32_t hash,
                             pdfium::span<const SkiaFontMap> font_map) {
  auto it = std::ranges::lower_bound(font_map, hash, std::less<>{},
                                     &SkiaFontMap::family);

  if (it != font_map.end() && it->family == hash) {
    return it->subst;
  }
  return nullptr;
}

bool SkiaMaybeArabic(ByteStringView facename) {
  ByteString name(facename);
  name.MakeLower();
  return name.Contains("arabic");
}

bool SkiaMaybeSymbol(ByteStringView facename) {
  ByteString name(facename);
  name.MakeLower();
  return name.Contains("symbol");
}

CFX_AndroidFontInfo::CFX_AndroidFontInfo() = default;

CFX_AndroidFontInfo::~CFX_AndroidFontInfo() = default;

bool CFX_AndroidFontInfo::Init(CFPF_SkiaFontMgr* font_mgr,
                               const char** user_paths) {
  if (!font_mgr) {
    return false;
  }

  font_mgr_ = font_mgr;
  font_mgr_->LoadFonts(user_paths);
  return true;
}

void CFX_AndroidFontInfo::EnumFontList(CFX_FontMapper* pMapper) {}

void* CFX_AndroidFontInfo::MapFont(int weight,
                                   bool bItalic,
                                   FX_Charset charset,
                                   int pitch_family,
                                   const ByteString& face) {
  if (!font_mgr_) {
    return nullptr;
  }

  uint32_t dwStyle = 0;
  if (weight >= 700) {
    dwStyle |= pdfium::kFontStyleForceBold;
  }
  if (bItalic) {
    dwStyle |= pdfium::kFontStyleItalic;
  }
  if (FontFamilyIsFixedPitch(pitch_family)) {
    dwStyle |= pdfium::kFontStyleFixedPitch;
  }
  if (FontFamilyIsScript(pitch_family)) {
    dwStyle |= pdfium::kFontStyleScript;
  }
  if (FontFamilyIsRoman(pitch_family)) {
    dwStyle |= pdfium::kFontStyleSerif;
  }
  return font_mgr_->CreateFont(face.AsStringView(), charset, dwStyle);
}

void* CFX_AndroidFontInfo::GetFont(const ByteString& face) {
  return nullptr;
}

size_t CFX_AndroidFontInfo::GetFontData(void* hFont,
                                        uint32_t table,
                                        pdfium::span<uint8_t> buffer) {
  if (!hFont) {
    return 0;
  }
  return static_cast<CFPF_SkiaFont*>(hFont)->GetFontData(table, buffer);
}

bool CFX_AndroidFontInfo::GetFaceName(void* hFont, ByteString* name) {
  if (!hFont) {
    return false;
  }
  *name = static_cast<CFPF_SkiaFont*>(hFont)->GetFamilyName();
  return true;
}

bool CFX_AndroidFontInfo::GetFontCharset(void* hFont, FX_Charset* charset) {
  if (!hFont) {
    return false;
  }
  *charset = static_cast<CFPF_SkiaFont*>(hFont)->GetCharset();
  return true;
}

void CFX_AndroidFontInfo::DeleteFont(void* hFont) {}
