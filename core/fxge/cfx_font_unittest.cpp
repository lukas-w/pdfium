// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_font.h"

#include <string>
#include <vector>

#include "core/fxge/cfx_cttgsubtable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"
#include "testing/utils/path_service.h"

TEST(CfxFontTest, GetCharCodesAndIndices) {
  std::string font_path = PathService::GetTestFilePath("fonts/ahem/Ahem.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false, 0));

  auto results = font.GetCharCodesAndIndices(0xffff);
  ASSERT_EQ(278u, results.size());
  EXPECT_EQ(CharCodeAndIndex(32, 3), results[0]);
  EXPECT_EQ(CharCodeAndIndex(33, 4), results[1]);
  EXPECT_EQ(CharCodeAndIndex(65279, 245), results[277]);
}

TEST(CfxFontTest, GetCharBBox) {
  std::string font_path = PathService::GetTestFilePath("fonts/ahem/Ahem.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false, 0));

  std::optional<FX_RECT> bbox = font.GetCharBBox(33, 4);
  ASSERT_TRUE(bbox.has_value());
  EXPECT_EQ(FX_RECT(0, 812, 1000, -199), *bbox);

  std::optional<FX_RECT> invalid_bbox =
      font.GetCharBBox(33, static_cast<uint32_t>(-1));
  ASSERT_TRUE(invalid_bbox.has_value());
  EXPECT_EQ(FX_RECT(0, 0, 0, 0), *invalid_bbox);
}

TEST(CfxFontTest, ParseGSUBTable) {
  std::string font_path =
      PathService::GetTestFilePath("fonts/bug_377948405.ttf");
  std::vector<uint8_t> bytes = GetFileContents(font_path.c_str());

  CFX_Font font;
  ASSERT_TRUE(font.LoadFaceZeroFromSpan(bytes, /*force_vertical=*/false, 0));

  auto gsub = font.ParseGSUBTable();
  ASSERT_TRUE(gsub);
  EXPECT_EQ(0u, gsub->GetVerticalGlyph(0));
  EXPECT_EQ(0u, gsub->GetVerticalGlyph(1));

  std::string sc_font_path = PathService::GetThirdPartyFilePath(
      "NotoSansCJK/NotoSansSC-Regular.subset.otf");
  std::vector<uint8_t> sc_bytes = GetFileContents(sc_font_path.c_str());

  CFX_Font sc_font;
  ASSERT_TRUE(
      sc_font.LoadFaceZeroFromSpan(sc_bytes, /*force_vertical=*/false, 0));

  auto sc_gsub = sc_font.ParseGSUBTable();
  ASSERT_TRUE(sc_gsub);
  EXPECT_EQ(0u, sc_gsub->GetVerticalGlyph(0));
  EXPECT_EQ(0u, sc_gsub->GetVerticalGlyph(1));
  EXPECT_EQ(10u, sc_gsub->GetVerticalGlyph(2));
  EXPECT_EQ(0u, sc_gsub->GetVerticalGlyph(3));
}
