// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/freetype/fx_freetype.h"

#include <stdint.h>

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

TEST(FXFreeTypeTest, VersionSupportsHinting) {
  ScopedFXFTLibraryRec library = InitializeFreeType();
  ASSERT_TRUE(library);
  EXPECT_TRUE(FreeTypeVersionSupportsHinting(library.get()));
}

TEST(FXFreeTypeTest, UnicodeFromAdobeName) {
  // Empty or invalid names.
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name(""));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("not_a_valid_name"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name(".notdef"));

  // uniXXXX format.
  EXPECT_EQ(0x0041, FXFT_unicode_from_adobe_name("uni0041"));
  EXPECT_EQ(0x0020, FXFT_unicode_from_adobe_name("uni0020"));
  EXPECT_EQ(0xFFFF, FXFT_unicode_from_adobe_name("uniFFFF"));
  // Invalid uni formats (non-hex, wrong length).
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("uni"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("uni004"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("uni004G"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("uni0041extra"));

  // uniXXXX with dot variant (bit 31 set).
  static constexpr uint32_t kVariantBit = 1U << 31;
  EXPECT_EQ(static_cast<int>(0x0041 | kVariantBit),
            FXFT_unicode_from_adobe_name("uni0041.swash"));

  // uXXXX format (4 to 6 hex digits).
  EXPECT_EQ(0x0041, FXFT_unicode_from_adobe_name("u0041"));
  EXPECT_EQ(0x1F600, FXFT_unicode_from_adobe_name("u1F600"));
  EXPECT_EQ(0x10FFFF, FXFT_unicode_from_adobe_name("u10FFFF"));
  // Invalid u formats.
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("u1"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("u12"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("u123"));
  EXPECT_EQ(0, FXFT_unicode_from_adobe_name("u0041G"));

  // uXXXX with dot variant.
  EXPECT_EQ(static_cast<int>(0x0041 | kVariantBit),
            FXFT_unicode_from_adobe_name("u0041.alt"));
  EXPECT_EQ(static_cast<int>(0x1F600 | kVariantBit),
            FXFT_unicode_from_adobe_name("u1F600.variant"));

  // Standard Adobe glyph names.
  EXPECT_EQ(0x0041, FXFT_unicode_from_adobe_name("A"));
  EXPECT_EQ(0x0020, FXFT_unicode_from_adobe_name("space"));
  EXPECT_EQ(0x0026, FXFT_unicode_from_adobe_name("ampersand"));

  // Standard Adobe glyph names with dot variant.
  EXPECT_EQ(static_cast<int>(0x0041 | kVariantBit),
            FXFT_unicode_from_adobe_name("A.swash"));
}

TEST(FXFreeTypeTest, AdobeNameFromUnicode) {
  std::array<char, 64> buffer;

  FXFT_adobe_name_from_unicode(buffer, L'A');
  EXPECT_STREQ("A", buffer.data());

  FXFT_adobe_name_from_unicode(buffer, L' ');
  EXPECT_STREQ("space", buffer.data());

  FXFT_adobe_name_from_unicode(buffer, L'&');
  EXPECT_STREQ("ampersand", buffer.data());

  // Non-existent unicode point.
  FXFT_adobe_name_from_unicode(buffer, 0xFFFF);
  EXPECT_EQ('\0', buffer[0]);
}
