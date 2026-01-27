// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_fontmgr.h"

#include <array>
#include <iterator>
#include <memory>
#include <utility>

#include "core/fxcrt/check_op.h"
#include "core/fxcrt/fixed_size_data_vector.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/cfx_substfont.h"
#include "core/fxge/fontdata/chromefontdata/chromefontdata.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/systemfontinfo_iface.h"

namespace {

constexpr std::array<pdfium::span<const uint8_t>,
                     CFX_FontMapper::kNumStandardFonts>
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

}  // namespace

CFX_FontMgr::FontDesc::FontDesc(FixedSizeDataVector<uint8_t> data)
    : font_data_(std::move(data)) {}

CFX_FontMgr::FontDesc::~FontDesc() = default;

void CFX_FontMgr::FontDesc::SetFace(uint32_t face_index, CFX_Face* face) {
  CHECK_LT(face_index, std::size(ttc_faces_));
  ttc_faces_[face_index].Reset(face);
}

CFX_Face* CFX_FontMgr::FontDesc::GetFace(uint32_t face_index) const {
  CHECK_LT(face_index, std::size(ttc_faces_));
  return ttc_faces_[face_index].Get();
}

CFX_FontMgr::CFX_FontMgr()
    : ft_library_(InitializeFreeType()),
      builtin_mapper_(std::make_unique<CFX_FontMapper>(this)),
      ft_library_supports_hinting_(
          FreeTypeSetLcdFilterMode(ft_library_.get()) ||
          FreeTypeVersionSupportsHinting(ft_library_.get())) {}

CFX_FontMgr::~CFX_FontMgr() = default;

RetainPtr<CFX_FontMgr::FontDesc> CFX_FontMgr::GetCachedFontDesc(
    const ByteString& face_name,
    int weight,
    bool bItalic) {
  auto it = face_map_.find({face_name, weight, bItalic});
  return it != face_map_.end() ? pdfium::WrapRetain(it->second.Get()) : nullptr;
}

RetainPtr<CFX_FontMgr::FontDesc> CFX_FontMgr::AddCachedFontDesc(
    const ByteString& face_name,
    int weight,
    bool bItalic,
    FixedSizeDataVector<uint8_t> data) {
  auto font_desc = pdfium::MakeRetain<FontDesc>(std::move(data));
  face_map_[{face_name, weight, bItalic}].Reset(font_desc.Get());
  return font_desc;
}

RetainPtr<CFX_FontMgr::FontDesc> CFX_FontMgr::GetCachedTTCFontDesc(
    size_t ttc_size,
    uint32_t checksum) {
  auto it = ttc_face_map_.find({ttc_size, checksum});
  return it != ttc_face_map_.end() ? pdfium::WrapRetain(it->second.Get())
                                   : nullptr;
}

RetainPtr<CFX_FontMgr::FontDesc> CFX_FontMgr::AddCachedTTCFontDesc(
    size_t ttc_size,
    uint32_t checksum,
    FixedSizeDataVector<uint8_t> data) {
  auto pNewDesc = pdfium::MakeRetain<FontDesc>(std::move(data));
  ttc_face_map_[{ttc_size, checksum}].Reset(pNewDesc.Get());
  return pNewDesc;
}

RetainPtr<CFX_Face> CFX_FontMgr::NewFixedFace(RetainPtr<FontDesc> desc,
                                              pdfium::span<const uint8_t> span,
                                              uint32_t face_index) {
  return CFX_Face::New(this, std::move(desc), span, face_index);
}

// static
pdfium::span<const uint8_t> CFX_FontMgr::GetStandardFont(size_t index) {
  return kFoxitFonts[index];
}

// static
pdfium::span<const uint8_t> CFX_FontMgr::GetGenericSansFont() {
  return kGenericSansFont;
}

// static
pdfium::span<const uint8_t> CFX_FontMgr::GetGenericSerifFont() {
  return kGenericSerifFont;
}
