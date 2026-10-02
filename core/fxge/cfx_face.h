// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_FACE_H_
#define CORE_FXGE_CFX_FACE_H_

#include <stdint.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>

#include "build/build_config.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/observed_ptr.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/freetype/fx_freetype.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"

#if defined(PDF_USE_SKIA)
#include "third_party/skia/include/core/SkRefCnt.h"  // nogncheck
#endif

#if defined(PDF_ENABLE_FONTATIONS)
#include "third_party/rust/cxx/v1/cxx.h"
#endif  // defined(PDF_ENABLE_FONTATIONS)

class CFX_CTTGSUBTable;
class CFX_GlyphBitmap;
class CFX_Path;
class CFX_SubstFont;

#if defined(PDF_ENABLE_XFA)
class CFX_CTTNameTable;
#endif  // defined(PDF_ENABLE_XFA)

#if defined(PDF_USE_SKIA)
class SkTypeface;
#endif

#if defined(PDF_ENABLE_FONTATIONS)
struct SkrifaFontHolder;
#else
struct SkrifaFontHolder {};
#endif

namespace fxge {
enum class FontEncoding : uint32_t;
}  // namespace fxge

class CFX_Face final : public Retainable, public Observable {
 public:
  using CharMap = void*;

  // Note that this corresponds to the cmap header in fonts, and not the cmap
  // data in PDFs.
  struct CharMapIdPair {
    friend constexpr bool operator==(const CharMapIdPair&,
                                     const CharMapIdPair&) = default;

    uint16_t platform_id;
    uint16_t encoding_id;
  };

  // Aliases for some commonly used cmaps.
  static constexpr CharMapIdPair kMacRomanCharMapIdPair{
      .platform_id = kPlatformMac,
      .encoding_id = kMacEncodingRoman};
  static constexpr CharMapIdPair kWindowsSymbolCharMapIdPair{
      .platform_id = kPlatformWindows,
      .encoding_id = kWindowsEncodingSymbol};
  static constexpr CharMapIdPair kWindowsUnicodeCharMapIdPair{
      .platform_id = kPlatformWindows,
      .encoding_id = kWindowsEncodingUnicode};

  static RetainPtr<CFX_Face> New(RetainPtr<Retainable> cache_entry,
                                 RetainPtr<CFX_ReadOnlySpanStream> font_stream,
                                 uint32_t face_index);
  static wchar_t UnicodeFromAdobeName(const char* name);
  static ByteString AdobeNameFromUnicode(wchar_t unicode);

  bool HasGlyphNames() const;
  bool IsTtOt() const;
  ByteString GetFontFormat();
  bool IsFixedWidth() const;
  bool IsItalic() const;
  bool IsBold() const;

  ByteString GetFamilyName() const;
  ByteString GetStyleName() const;

  FX_RECT GetBBox() const;
  uint16_t GetUnitsPerEm() const;
  int EmAdjust(int value) const;
  int16_t GetAscender() const;
  int16_t GetDescender() const;

  pdfium::span<const uint8_t> GetData() const;

  std::unique_ptr<CFX_CTTGSUBTable> ParseGSUBTable();

  int GetGlyphCount() const;
  FX_RECT GetGlyphBBox(uint32_t glyph_index) const;
  std::optional<FX_RECT> GetFontGlyphBBox(uint32_t glyph_index);
  std::unique_ptr<CFX_GlyphBitmap> RenderGlyph(uint32_t glyph_index,
                                               bool is_cid_font,
                                               bool is_vertical,
                                               const CFX_Matrix& matrix,
                                               int dest_width,
                                               FontAntiAliasingMode anti_alias,
                                               const CFX_SubstFont* subst_font);
  std::unique_ptr<CFX_Path> LoadGlyphPath(uint32_t glyph_index,
                                          int dest_width,
                                          bool is_vertical,
                                          const CFX_SubstFont* subst_font);
  int GetGlyphTTWidth(uint32_t glyph_index) const;
  int GetGlyphWidth(uint32_t glyph_index,
                    int dest_width,
                    int weight,
                    const CFX_SubstFont* subst_font);
  ByteString GetGlyphName(uint32_t glyph_index);

  int GetCharIndex(uint32_t code);
  int GetNameIndex(const char* name);

  FX_RECT GetCharBBox(uint32_t code, int glyph_index);

  std::vector<CharCodeAndIndex> GetCharCodesAndIndices(char32_t max_char);

  CharMap GetCurrentCharMap() const;
  std::optional<fxge::FontEncoding> GetCurrentCharMapEncoding() const;
  CharMapIdPair GetCharMapIdPairByIndex(size_t index) const;
  uint16_t GetCharMapPlatformIdByIndex(size_t index) const;
  fxge::FontEncoding GetCharMapEncodingByIndex(size_t index) const;
  size_t GetCharMapCount() const;
  int LoadGlyph(uint32_t glyph_index, bool scale);
  ByteString GetPostscriptName();
  void SetCharMap(CharMap map);
  void SetCharMapByIndex(size_t index);
  bool SelectCharMap(fxge::FontEncoding encoding);

#if defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_LINUX)
  // Returns enum FontStyle values.
  uint32_t GetFontStyle();

  std::optional<std::array<uint32_t, 2>> GetOs2CodePageRange();
#endif

#if defined(PDF_ENABLE_XFA)
  bool IsScalable() const;
  int GetNumFaces() const;
  std::unique_ptr<CFX_CTTNameTable> ParseNameTable();
  std::optional<std::array<uint32_t, 4>> GetOs2UnicodeRange();
#endif

#if BUILDFLAG(IS_WIN)
  bool CanEmbed();
#endif

#if defined(PDF_USE_SKIA)
  SkTypeface* GetOrCreateSkTypeface();
#endif

 private:
  CFX_Face(RetainPtr<Retainable> cache_entry,
           RetainPtr<CFX_ReadOnlySpanStream> font_stream,
           FT_FaceRec* rec,
           std::unique_ptr<SkrifaFontHolder> skrifa_font);

  ~CFX_Face() override;

  FT_FaceRec* GetRec() { return rec_.get(); }
  const FT_FaceRec* GetRec() const { return rec_.get(); }

  uint16_t GetCharMapEncodingIdByIndex(size_t index) const;
  CFX_Size GetPixelSize() const;

  bool IsTricky() const;
  void AdjustVariationParams(int glyph_index, int dest_width, int weight);
#if defined(PDF_ENABLE_FONTATIONS)
  void AdjustSubstFontTransform(const CFX_SubstFont* subst_font,
                                int dest_width,
                                float advance_width,
                                bool is_cid_font,
                                bool is_vertical,
                                CFX_Matrix* matrix) const;
#endif

  pdfium::span<const FT_CharMap> GetCharMaps() const;

  // Returns the size of the data, or 0 on failure. Only write into `buffer` if
  // it is large enough to hold the data.
  size_t GetSfntTable(uint32_t table, pdfium::span<uint8_t> buffer);

#if defined(PDF_ENABLE_XFA) || BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_LINUX)
  std::optional<std::array<uint8_t, 2>> GetOs2Panose();
#endif

  // `cache_entry_` must outlive `font_stream_`. Faces managed by a cache
  // and sharing the same `font_stream_` keep the cache entry that indexes
  // that stream alive via this member while there is at least one face
  // using it. This may be nullptr for faces not managed by a cache.
  RetainPtr<Retainable> cache_entry_;

  // `font_stream_` must outlive `rec_` and `skia_typeface_`. Faces keep
  // the actual data backing the `rec_` and `skia_typeface_` alive via
  // this member while the `rec_` and `skia_typeface_` is still using it.
  RetainPtr<CFX_ReadOnlySpanStream> font_stream_;

  ScopedFXFTFaceRec const rec_;
#if defined(PDF_USE_SKIA)
  sk_sp<SkTypeface> skia_typeface_;
#endif  // defined(PDF_USE_SKIA)
#if defined(PDF_ENABLE_FONTATIONS)
  std::unique_ptr<SkrifaFontHolder> const skrifa_font_;
#endif  // defined(PDF_ENABLE_FONTATIONS)
  fxge::FontEncoding selected_encoding_ = fxge::FontEncoding::kNone;
};

#endif  // CORE_FXGE_CFX_FACE_H_
