// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_font.h"

#include <string>
#include <vector>

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
