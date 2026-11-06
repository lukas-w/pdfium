// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_cttnametable.h"

#include <stdint.h>

#include <array>

#include "core/fxcrt/widestring.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

TEST(CFX_CTTNameTableTest, Empty) {
  CFX_CTTNameTable table;
  EXPECT_TRUE(table.GetFamilyNames().empty());

  CFX_CTTNameTable empty_table(pdfium::span<const uint8_t>{});
  EXPECT_TRUE(empty_table.GetFamilyNames().empty());
}

TEST(CFX_CTTNameTableTest, TruncatedHeader) {
  static constexpr uint8_t kTruncatedHeader[] = {0x00, 0x00, 0x00, 0x01};
  CFX_CTTNameTable table(kTruncatedHeader);
  EXPECT_TRUE(table.GetFamilyNames().empty());
}

TEST(CFX_CTTNameTableTest, InvalidStringOffset) {
  static constexpr uint8_t kInvalidStringOffset[] = {
      0x00, 0x00,  // format
      0x00, 0x01,  // count = 1
      0xFF, 0xFF,  // string_offset = 65535 (out of bounds)
  };
  CFX_CTTNameTable table(kInvalidStringOffset);
  EXPECT_TRUE(table.GetFamilyNames().empty());
}

TEST(CFX_CTTNameTableTest, AddFamilyName) {
  CFX_CTTNameTable table;
  table.AddFamilyName(L"First");
  table.AddFamilyName(L"Second");
  ASSERT_THAT(table.GetFamilyNames(), ElementsAre(L"First", L"Second"));
}

TEST(CFX_CTTNameTableTest, ParseMacintoshRecord) {
  // Minimal TT name table with 1 name record for Macintosh (Platform 1).
  // Header:
  //   format: 0 (2 bytes)
  //   count: 1 (2 bytes)
  //   string_offset: 18 (2 bytes)
  // NameRecord 0 (12 bytes at offset 6):
  //   platform_id: 1 (2 bytes)
  //   encoding_id: 0 (2 bytes)
  //   language_id: 0 (2 bytes)
  //   name_id: 1 (2 bytes)
  //   length: 4 (2 bytes)
  //   offset: 0 (2 bytes)
  // String storage (starts at offset 18):
  //   "Test" (4 bytes, testing exact bounds without trailing padding)
  static constexpr uint8_t kTableData[] = {
      0x00, 0x00,           // format
      0x00, 0x01,           // count = 1
      0x00, 0x12,           // string_offset = 18
      0x00, 0x01,           // platform_id = 1
      0x00, 0x00,           // encoding_id = 0
      0x00, 0x00,           // language_id = 0
      0x00, 0x01,           // name_id = 1
      0x00, 0x04,           // length = 4
      0x00, 0x00,           // offset = 0
      'T',  'e',  's', 't'  // string data
  };

  CFX_CTTNameTable table(kTableData);
  EXPECT_THAT(table.GetFamilyNames(), ElementsAre(L"Test"));
}

TEST(CFX_CTTNameTableTest, ParseWindowsRecord) {
  // Minimal TT name table with 1 name record for Windows (Platform 3).
  // Uses big-endian UTF-16 encoding.
  // Header:
  //   format: 0 (2 bytes)
  //   count: 1 (2 bytes)
  //   string_offset: 18 (2 bytes)
  // NameRecord 0 (12 bytes at offset 6):
  //   platform_id: 3 (2 bytes)
  //   encoding_id: 1 (2 bytes)
  //   language_id: 0x0409 (2 bytes)
  //   name_id: 1 (2 bytes)
  //   length: 8 (2 bytes)
  //   offset: 0 (2 bytes)
  // String storage (starts at offset 18):
  //   UTF-16BE "Test" (8 bytes)
  static constexpr uint8_t kTableData[] = {
      0x00, 0x00,                                  // format
      0x00, 0x01,                                  // count = 1
      0x00, 0x12,                                  // string_offset = 18
      0x00, 0x03,                                  // platform_id = 3 (Windows)
      0x00, 0x01,                                  // encoding_id = 1
      0x04, 0x09,                                  // language_id = 0x0409
      0x00, 0x01,                                  // name_id = 1 (Family name)
      0x00, 0x08,                                  // length = 8 bytes
      0x00, 0x00,                                  // offset = 0
      0x00, 'T',  0x00, 'e', 0x00, 's', 0x00, 't'  // string data
  };

  CFX_CTTNameTable table(kTableData);
  ASSERT_EQ(1u, table.GetFamilyNames().size());
  EXPECT_EQ(L"Test", table.GetFamilyNames()[0]);
}

TEST(CFX_CTTNameTableTest, ParseNonFamilyNameRecord) {
  // Name record with name_id != 1 (e.g. name_id = 2 for Subfamily/Style).
  static constexpr uint8_t kTableData[] = {
      0x00, 0x00,  // format
      0x00, 0x01,  // count = 1
      0x00, 0x12,  // string_offset = 18
      0x00, 0x03,  // platform_id = 3
      0x00, 0x01,  // encoding_id = 1
      0x04, 0x09,  // language_id = 0x0409
      0x00, 0x02,  // name_id = 2 (not family name)
      0x00, 0x08,  // length = 8
      0x00, 0x00,  // offset = 0
      0x00, 'B',  0x00, 'o', 0x00, 'l', 0x00, 'd'  // "Bold"
  };

  CFX_CTTNameTable table(kTableData);
  EXPECT_TRUE(table.GetFamilyNames().empty());
}

TEST(CFX_CTTNameTableTest, ParseOutOfBoundsRecord) {
  // Offset points beyond string storage.
  static constexpr uint8_t kTableData[] = {
      0x00, 0x00,           // format
      0x00, 0x01,           // count = 1
      0x00, 0x12,           // string_offset = 18
      0x00, 0x01,           // platform_id = 1
      0x00, 0x00,           // encoding_id = 0
      0x00, 0x00,           // language_id = 0
      0x00, 0x01,           // name_id = 1
      0x00, 0x04,           // length = 4
      0x00, 0x20,           // offset = 32 (out of bounds)
      'T',  'e',  's', 't'  // string data
  };

  CFX_CTTNameTable table(kTableData);
  EXPECT_TRUE(table.GetFamilyNames().empty());
}
