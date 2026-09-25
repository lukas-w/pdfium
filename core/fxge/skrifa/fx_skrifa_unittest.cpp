// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxcrt/compiler_specific.h"
#include "core/fxge/cfx_font.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_glyphbitmap.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/fontdata/chromefontdata/chromefontdata.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"
#include "core/fxge/skrifa/src/main.rs.h"
#include "core/fxge/skrifa/src/outlines.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

TEST(FxSkrifaTest, TestFoxitFixedCff) {
  std::string font_path = PathService::GetTestFilePath("fonts/foxit_fixed.cff");
  skrifa::run(rust::Str(font_path));
}

TEST(FxSkrifaTest, TestGetOs2CodePageRange) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  skrifa::CodePageRange range;
  EXPECT_TRUE(font->get_os2_code_page_range(range));
  EXPECT_EQ(range.range1, 0u);
  EXPECT_EQ(range.range2, 0u);
}

TEST(FxSkrifaTest, TestGetOs2Panose) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  skrifa::Os2Panose panose;
  EXPECT_TRUE(font->get_os2_panose(panose));
  EXPECT_EQ(panose.b0, 2);
  EXPECT_EQ(panose.b1, 0);
}

TEST(FxSkrifaTest, TestGetOs2FsType) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  uint16_t fs_type = 0x1234;  // Show that is is updated.
  EXPECT_TRUE(font->get_os2_fs_type(fs_type));
  EXPECT_EQ(fs_type, 0u);
}

TEST(FxSkrifaTest, TestGetCharCodesAndIndices) {
  std::string font_path = PathService::GetTestFilePath("fonts/ahem/Ahem.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  auto results = font->get_char_codes_and_indices(0xFFFF);
  ASSERT_EQ(278u, results.size());
  EXPECT_EQ(skrifa::CharCodeAndIndex(32, 3), results[0]);
  EXPECT_EQ(skrifa::CharCodeAndIndex(33, 4), results[1]);
  EXPECT_EQ(skrifa::CharCodeAndIndex(65279, 245), results[277]);
}

TEST(FxSkrifaTest, TestIsTricky) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  EXPECT_FALSE(font->is_tricky());
}

TEST(FxSkrifaTest, TestGetNumFaces) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  EXPECT_EQ(skrifa::get_num_faces(rust::Slice<const uint8_t>(bytes)), 1u);
}

TEST(FxSkrifaTest, TestHasOutline) {
  std::string font_path = PathService::GetTestFilePath("fonts/bug_2094.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  auto font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(font->is_ok());

  EXPECT_TRUE(font->has_outline(0));
}

TEST(FxSkrifaTest, TestRobotoGlyph167Bounds) {
  std::string font_path = PathService::GetTestFilePath(
      "../../third_party/harfbuzz/src/perf/fonts/Roboto-Regular.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  ASSERT_EQ(FT_Load_Glyph(face->GetFTFaceForTesting(), 167, FT_LOAD_NO_SCALE),
            0);
  FX_RECT ft_bbox = face->GetGlyphBBox(167);

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());
  skrifa::BoundingBox skrifa_bbox = skrifa_font->glyph_bounds(167);

  uint16_t upem = face->GetUnitsPerEm();
  EXPECT_EQ(upem, 2048);

  FX_RECT skrifa_rect(NormalizeFontMetric(skrifa_bbox.x_min, upem),
                      NormalizeFontMetric(skrifa_bbox.y_max, upem),
                      NormalizeFontMetric(skrifa_bbox.x_max, upem),
                      NormalizeFontMetric(skrifa_bbox.y_min, upem));

  EXPECT_TRUE(ft_bbox.Near(skrifa_rect, 1));

  std::optional<FX_RECT> font_glyph_bbox = face->GetFontGlyphBBox(167);
  ASSERT_TRUE(font_glyph_bbox.has_value());
  FX_RECT expected_rect(NormalizeFontMetric(skrifa_bbox.x_min, upem),
                        NormalizeFontMetric(skrifa_bbox.y_min, upem),
                        NormalizeFontMetric(skrifa_bbox.x_max, upem),
                        NormalizeFontMetric(skrifa_bbox.y_max, upem));
  EXPECT_TRUE(font_glyph_bbox->Near(expected_rect, 1));
}

TEST(FxSkrifaTest, TestMinionCff) {
  std::string font_path = PathService::GetTestFilePath("fonts/minion.cff");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());

  EXPECT_EQ(face->GetFTFaceForTesting()->num_glyphs,
            static_cast<long>(skrifa_font->num_glyphs()));

  auto family_name = skrifa_font->family_name();
  auto ps_name = skrifa_font->postscript_name();

  ByteString ft_family = face->GetFamilyName();
  ByteString sk_family =
      UNSAFE_BUFFERS(ByteString(family_name.data(), family_name.size()));
  MaybeRemoveSubsettedFontPrefix(ft_family);
  MaybeRemoveSubsettedFontPrefix(sk_family);
  EXPECT_EQ(ft_family, sk_family);

  ByteString ft_ps = face->GetPostscriptName();
  ByteString sk_ps = UNSAFE_BUFFERS(ByteString(ps_name.data(), ps_name.size()));
  MaybeRemoveSubsettedFontPrefix(ft_ps);
  MaybeRemoveSubsettedFontPrefix(sk_ps);
  EXPECT_EQ(ft_ps, sk_ps);

  EXPECT_TRUE(skrifa_font->has_outline(55));
  EXPECT_EQ(FT_Load_Glyph(face->GetFTFaceForTesting(), 55, FT_LOAD_NO_SCALE),
            0);
}

TEST(FxSkrifaTest, TestTimesBoldGlyph104Bounds) {
  std::string font_path = PathService::GetTestFilePath("fonts/times_bold.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  FT_Face ft_face = face->GetFTFaceForTesting();
  ASSERT_EQ(FT_Load_Glyph(ft_face, 104, FT_LOAD_NO_SCALE), 0);
  FT_Glyph_Metrics ft_metrics = ft_face->glyph->metrics;

  int ft_x_min = ft_metrics.horiBearingX;
  int ft_y_max = ft_metrics.horiBearingY;
  int ft_x_max = ft_metrics.horiBearingX + ft_metrics.width;
  int ft_y_min = ft_metrics.horiBearingY - ft_metrics.height;

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());

  skrifa::BoundingBox skrifa_bbox = skrifa_font->glyph_bounds(104);
  EXPECT_EQ(ft_x_min, skrifa_bbox.x_min);
  EXPECT_EQ(ft_y_min, skrifa_bbox.y_min);
  EXPECT_EQ(ft_x_max, skrifa_bbox.x_max);
  EXPECT_EQ(ft_y_max, skrifa_bbox.y_max);
}

TEST(FxSkrifaTest, TestRobotoGlyph2344Bounds) {
  std::string font_path = PathService::GetTestFilePath("fonts/roboto.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());
  ASSERT_FALSE(bytes.empty());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false,
                                        /*object_tag=*/0));
  auto face = font.GetFace();
  ASSERT_TRUE(face);

  FT_Face ft_face = face->GetFTFaceForTesting();
  ASSERT_EQ(FT_Load_Glyph(ft_face, 2344, FT_LOAD_NO_SCALE), 0);
  FT_Glyph_Metrics ft_metrics = ft_face->glyph->metrics;

  int ft_x_min = ft_metrics.horiBearingX;
  int ft_y_max = ft_metrics.horiBearingY;
  int ft_x_max = ft_metrics.horiBearingX + ft_metrics.width;
  int ft_y_min = ft_metrics.horiBearingY - ft_metrics.height;

  auto skrifa_font = skrifa::new_font(rust::Slice<const uint8_t>(bytes), 0);
  ASSERT_TRUE(skrifa_font->is_ok());

  skrifa::BoundingBox skrifa_bbox = skrifa_font->glyph_bounds(2344);
  EXPECT_EQ(ft_x_min, skrifa_bbox.x_min);
  EXPECT_EQ(ft_y_min, skrifa_bbox.y_min);
  EXPECT_EQ(ft_x_max, skrifa_bbox.x_max);
  EXPECT_EQ(ft_y_max, skrifa_bbox.y_max);
}

TEST(FxSkrifaTest, TestType1Face) {
  EXPECT_EQ(UnicodeFromAdobeName("period"), L'.');
  EXPECT_EQ(UnicodeFromAdobeName("A"), L'A');
  EXPECT_EQ(AdobeNameFromUnicode(L'.'), "period");
  EXPECT_EQ(AdobeNameFromUnicode(L'A'), "A");

  if (!CFX_GEModule::IsFontations()) {
    return;
  }

  auto stream =
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kFoxitSansMMFontData);
  auto face = CFX_Face::New(nullptr, stream, 0);
  ASSERT_TRUE(face);

  EXPECT_EQ(face->GetFTFaceForTesting(), nullptr);
  EXPECT_EQ(face->GetFontFormat(), "Type 1");
  EXPECT_EQ(face->GetFamilyName(), "Chrome Sans MM");
  EXPECT_EQ(face->GetPostscriptName(), "ChromeSansMM");
  EXPECT_EQ(face->GetGlyphCount(), 230);
  EXPECT_EQ(face->GetUnitsPerEm(), 1000);

  EXPECT_TRUE(face->SelectCharMap(fxge::FontEncoding::kAdobeCustom));
  int gid_x = face->GetCharIndex(120);
  EXPECT_EQ(gid_x, 103);

  EXPECT_TRUE(face->SelectCharMap(fxge::FontEncoding::kUnicode));
  EXPECT_EQ(face->GetCharIndex(L'x'), gid_x);

  std::unique_ptr<CFX_Path> path =
      face->LoadGlyphPath(gid_x, 0, false, nullptr);
  ASSERT_TRUE(path);
  EXPECT_EQ(path->GetPoints().size(), 12u);

  auto glyph_bitmap =
      face->RenderGlyph(gid_x, /*is_cid_font=*/false, /*is_vertical=*/false,
                        CFX_Matrix(12.0f, 0, 0, 12.0f, 0, 0), /*dest_width=*/0,
                        FontAntiAliasingMode::kNormal, /*subst_font=*/nullptr);
  ASSERT_TRUE(glyph_bitmap);
  EXPECT_TRUE(glyph_bitmap->GetBitmap());
  EXPECT_EQ(glyph_bitmap->GetBitmap()->GetWidth(), 6);
  EXPECT_EQ(glyph_bitmap->GetBitmap()->GetHeight(), 7);

  auto glyph_lcd =
      face->RenderGlyph(gid_x, /*is_cid_font=*/false, /*is_vertical=*/false,
                        CFX_Matrix(12.0f, 0, 0, 12.0f, 0, 0), /*dest_width=*/0,
                        FontAntiAliasingMode::kLcd, /*subst_font=*/nullptr);
  ASSERT_TRUE(glyph_lcd);
  EXPECT_TRUE(glyph_lcd->GetBitmap());
  EXPECT_EQ(glyph_lcd->GetBitmap()->GetWidth(), 18);
  EXPECT_EQ(glyph_lcd->GetBitmap()->GetHeight(), 7);

  auto glyph_mono =
      face->RenderGlyph(gid_x, /*is_cid_font=*/false, /*is_vertical=*/false,
                        CFX_Matrix(12.0f, 0, 0, 12.0f, 0, 0), /*dest_width=*/0,
                        FontAntiAliasingMode::kMono, /*subst_font=*/nullptr);
  ASSERT_TRUE(glyph_mono);
  EXPECT_TRUE(glyph_mono->GetBitmap());
  EXPECT_EQ(glyph_mono->GetBitmap()->GetWidth(), 6);
  EXPECT_EQ(glyph_mono->GetBitmap()->GetHeight(), 7);
}
