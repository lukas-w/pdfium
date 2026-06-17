// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file tests the cross-platform font mapping and selection logic of
// CFPF_SkiaFontMgr. It is compiled and run on Linux host tests as the
// logic does not depend on Android-specific system APIs.

#include "core/fxge/android/cfpf_skiafontmgr.h"

#include <string>
#include <utility>
#include <vector>

#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxge/android/cfpf_skiafont.h"
#include "core/fxge/cfx_face.h"
#include "core/fxge/fx_font.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

class TestSkiaFontMgr : public CFPF_SkiaFontMgr {
 public:
  TestSkiaFontMgr() = default;

  RetainPtr<CFX_Face> GetFontFace(const ByteString& path,
                                  int32_t face_index) override {
    last_requested_path_ = path;
    return dummy_face_;
  }

  ByteString last_requested_path_;
  RetainPtr<CFX_Face> dummy_face_;
};

class CFPFSkiaFontMgrTest : public ::testing::Test {
 public:
  CFPFSkiaFontMgrTest() = default;

  void SetUp() override {
    std::string font_path = PathService::GetThirdPartyFilePath(
        "NotoSansCJK/NotoSansSC-Regular.subset.otf");
    ASSERT_FALSE(font_path.empty());
    font_data_ = GetFileContents(font_path.c_str());
    ASSERT_FALSE(font_data_.empty());

    auto stream = pdfium::MakeRetain<CFX_ReadOnlySpanStream>(font_data_);
    font_mgr_.dummy_face_ = CFX_Face::New(nullptr, std::move(stream), 0);
    ASSERT_TRUE(font_mgr_.dummy_face_);
  }

  CFPF_SkiaFont* CreateFont(ByteStringView family_name,
                            FX_Charset charset,
                            uint32_t style) {
    font_mgr_.last_requested_path_.clear();
    return font_mgr_.CreateFont(family_name, charset, style);
  }

  ByteString GetLastRequestedPath() const {
    return font_mgr_.last_requested_path_;
  }

  void AddDummyFont(const char* font_name,
                    const char* path,
                    FX_CharsetFlag charset,
                    uint32_t glyph_count = 0,
                    uint32_t style = 0) {
    auto entry = std::make_unique<CFPF_SkiaFontMgr::Entry>();
    entry->path = path;
    entry->family = font_name;
    entry->style = style;
    entry->face_index = 0;
    entry->charsets = charset;
    entry->glyph_num = glyph_count;
    font_mgr_.font_faces_.push_back(std::move(entry));
  }

 protected:
  std::vector<uint8_t> font_data_;  // Must outlive `font_mgr_`
  TestSkiaFontMgr font_mgr_;
};

TEST_F(CFPFSkiaFontMgrTest, CreateFontPreferOriginalNonCJK) {
  // Install both original (Arial) and substitution (Roboto).
  AddDummyFont("Arial", "path/to/Arial", FX_CharsetFlag::kANSI);
  AddDummyFont("Roboto", "path/to/Roboto", FX_CharsetFlag::kANSI);

  // Map "Arial" -> should prefer "Arial" over "Roboto" (subst).
  CFPF_SkiaFont* font = CreateFont("Arial", FX_Charset::kANSI, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/Arial");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontFallbackToSubstNonCJK) {
  // Install only substitution (Roboto).
  AddDummyFont("Roboto", "path/to/Roboto", FX_CharsetFlag::kANSI);

  // Map "Arial" -> should fallback to "Roboto" (subst for Arial).
  CFPF_SkiaFont* font = CreateFont("Arial", FX_Charset::kANSI, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/Roboto");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontPreferOriginalCJK) {
  // SimSun (original) wins over Droid Sans Fallback (subst with more glyphs)
  // because it is a perfect match (exact name match + style match), triggering
  // an early exit.
  AddDummyFont("SimSun", "path/to/SimSun", FX_CharsetFlag::kChineseSimplified,
               1000);
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/SimSun");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontCJKOrderDependency1) {
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000,
               pdfium::kFontStyleForceBold);
  AddDummyFont("SimSun", "path/to/SimSun", FX_CharsetFlag::kChineseSimplified,
               1000, pdfium::kFontStyleForceBold);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/DroidSansFallback");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontCJKOrderDependency2) {
  AddDummyFont("SimSun", "path/to/SimSun", FX_CharsetFlag::kChineseSimplified,
               1000, pdfium::kFontStyleForceBold);
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000,
               pdfium::kFontStyleForceBold);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/SimSun");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontFallbackToSubstCJK) {
  // Map "SimSun" -> should fallback to "Droid Sans Fallback".
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000);

  CFPF_SkiaFont* font = CreateFont("SimSun", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/DroidSansFallback");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontCJKFallbackNoMatch) {
  // Install some other CJK font (e.g. Droid Sans Fallback).
  AddDummyFont("Droid Sans Fallback", "path/to/DroidSansFallback",
               FX_CharsetFlag::kChineseSimplified, 2000);

  // Map some random CJK name that has no subst rule (e.g. "MyCJKFont").
  // It should find Droid Sans Fallback because it has more glyphs (2000 > 0).
  CFPF_SkiaFont* font =
      CreateFont("MyCJKFont", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/DroidSansFallback");
}

TEST_F(CFPFSkiaFontMgrTest, CreateFontCJKDifferentGlyphCountsNoNameMatch) {
  // Install two non-matching CJK fonts with different glyph counts.
  AddDummyFont("CJK Font 1", "path/to/CJKFont1",
               FX_CharsetFlag::kChineseSimplified, 1000);
  AddDummyFont("CJK Font 2", "path/to/CJKFont2",
               FX_CharsetFlag::kChineseSimplified, 2000);

  // Map "MyCJKFont" (no match) -> should prefer CJK Font 2 (more glyphs).
  // Reversed iteration sees CJK Font 2 first (best_glyph_num = 2000).
  // Then CJK Font 1 (glyph_num = 1000 < 2000) -> not selected.
  // If we added them in opposite order, CJK Font 2 still wins.
  CFPF_SkiaFont* font =
      CreateFont("MyCJKFont", FX_Charset::kChineseSimplified, 0);
  ASSERT_TRUE(font);
  EXPECT_EQ(GetLastRequestedPath(), "path/to/CJKFont2");
}
