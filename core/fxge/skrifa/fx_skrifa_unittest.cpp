// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "core/fxge/cfx_font.h"
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
