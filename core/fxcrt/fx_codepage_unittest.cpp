// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/fx_codepage.h"

#include <array>

#include "build/build_config.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/widestring.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

TEST(FXCodePageTest, GetACP) {
#if BUILDFLAG(IS_WIN)
  EXPECT_NE(FX_CodePage::kFailure, FX_GetACP());
#else
  EXPECT_EQ(FX_CodePage::kDefANSI, FX_GetACP());
#endif
}

TEST(FXCodePageTest, GetCodePageFromCharset) {
  EXPECT_EQ(FX_CodePage::kMSWin_WesternEuropean,
            FX_GetCodePageFromCharset(FX_Charset::kANSI));
  EXPECT_EQ(FX_CodePage::kShiftJIS,
            FX_GetCodePageFromCharset(FX_Charset::kShiftJIS));
  EXPECT_EQ(FX_CodePage::kHangul,
            FX_GetCodePageFromCharset(FX_Charset::kHangul));
  EXPECT_EQ(FX_CodePage::kChineseSimplified,
            FX_GetCodePageFromCharset(FX_Charset::kChineseSimplified));
  EXPECT_EQ(FX_CodePage::kChineseTraditional,
            FX_GetCodePageFromCharset(FX_Charset::kChineseTraditional));
  EXPECT_EQ(FX_CodePage::kMSWin_Cyrillic,
            FX_GetCodePageFromCharset(FX_Charset::kMSWin_Cyrillic));
  EXPECT_EQ(FX_CodePage::kMSDOS_Thai,
            FX_GetCodePageFromCharset(FX_Charset::kThai));
  EXPECT_EQ(FX_CodePage::kMSWin_EasternEuropean,
            FX_GetCodePageFromCharset(FX_Charset::kMSWin_EasternEuropean));

  // Invalid charset returns kFailure.
  EXPECT_EQ(FX_CodePage::kFailure,
            FX_GetCodePageFromCharset(static_cast<FX_Charset>(253)));
}

TEST(FXCodePageTest, GetCharsetFromCodePage) {
  EXPECT_EQ(FX_Charset::kANSI,
            FX_GetCharsetFromCodePage(FX_CodePage::kMSWin_WesternEuropean));
  EXPECT_EQ(FX_Charset::kShiftJIS,
            FX_GetCharsetFromCodePage(FX_CodePage::kShiftJIS));
  EXPECT_EQ(FX_Charset::kHangul,
            FX_GetCharsetFromCodePage(FX_CodePage::kHangul));
  EXPECT_EQ(FX_Charset::kChineseSimplified,
            FX_GetCharsetFromCodePage(FX_CodePage::kChineseSimplified));
  EXPECT_EQ(FX_Charset::kChineseTraditional,
            FX_GetCharsetFromCodePage(FX_CodePage::kChineseTraditional));
  EXPECT_EQ(FX_Charset::kMSWin_Cyrillic,
            FX_GetCharsetFromCodePage(FX_CodePage::kMSWin_Cyrillic));
  EXPECT_EQ(FX_Charset::kThai,
            FX_GetCharsetFromCodePage(FX_CodePage::kMSDOS_Thai));
  EXPECT_EQ(FX_Charset::kMSWin_EasternEuropean,
            FX_GetCharsetFromCodePage(FX_CodePage::kMSWin_EasternEuropean));

  // Unknown codepage defaults to kANSI.
  EXPECT_EQ(FX_Charset::kANSI,
            FX_GetCharsetFromCodePage(FX_CodePage::kFailure));
}

TEST(FXCodePageTest, GetCharsetFromInt) {
  EXPECT_EQ(FX_Charset::kANSI, FX_GetCharsetFromInt(0));
  EXPECT_EQ(FX_Charset::kDefault, FX_GetCharsetFromInt(1));
  EXPECT_EQ(FX_Charset::kSymbol, FX_GetCharsetFromInt(2));
  EXPECT_EQ(FX_Charset::kMAC_Roman, FX_GetCharsetFromInt(77));
  EXPECT_EQ(FX_Charset::kMAC_ShiftJIS, FX_GetCharsetFromInt(78));
  EXPECT_EQ(FX_Charset::kShiftJIS, FX_GetCharsetFromInt(128));
  EXPECT_EQ(FX_Charset::kHangul, FX_GetCharsetFromInt(129));
  EXPECT_EQ(FX_Charset::kJohab, FX_GetCharsetFromInt(130));
  EXPECT_EQ(FX_Charset::kChineseSimplified, FX_GetCharsetFromInt(134));
  EXPECT_EQ(FX_Charset::kChineseTraditional, FX_GetCharsetFromInt(136));
  EXPECT_EQ(FX_Charset::kMSWin_Greek, FX_GetCharsetFromInt(161));
  EXPECT_EQ(FX_Charset::kMSWin_Turkish, FX_GetCharsetFromInt(162));
  EXPECT_EQ(FX_Charset::kMSWin_Vietnamese, FX_GetCharsetFromInt(163));
  EXPECT_EQ(FX_Charset::kMSWin_Hebrew, FX_GetCharsetFromInt(177));
  EXPECT_EQ(FX_Charset::kMSWin_Arabic, FX_GetCharsetFromInt(178));
  EXPECT_EQ(FX_Charset::kMSWin_Baltic, FX_GetCharsetFromInt(186));
  EXPECT_EQ(FX_Charset::kMSWin_Cyrillic, FX_GetCharsetFromInt(204));
  EXPECT_EQ(FX_Charset::kThai, FX_GetCharsetFromInt(222));
  EXPECT_EQ(FX_Charset::kMSWin_EasternEuropean, FX_GetCharsetFromInt(238));
  EXPECT_EQ(FX_Charset::kOEM, FX_GetCharsetFromInt(255));

  // Invalid values return kANSI.
  EXPECT_EQ(FX_Charset::kANSI, FX_GetCharsetFromInt(-1));
  EXPECT_EQ(FX_Charset::kANSI, FX_GetCharsetFromInt(999));
}

TEST(FXCodePageTest, CharsetFlagForCharset) {
  EXPECT_EQ(FX_CharsetFlag::kANSI, FX_CharsetFlagForCharset(FX_Charset::kANSI));
  EXPECT_EQ(FX_CharsetFlag::kSymbol,
            FX_CharsetFlagForCharset(FX_Charset::kSymbol));
  EXPECT_EQ(FX_CharsetFlag::kShiftJIS,
            FX_CharsetFlagForCharset(FX_Charset::kShiftJIS));
  EXPECT_EQ(FX_CharsetFlag::kHangul,
            FX_CharsetFlagForCharset(FX_Charset::kHangul));
  EXPECT_EQ(FX_CharsetFlag::kJohab,
            FX_CharsetFlagForCharset(FX_Charset::kJohab));
  EXPECT_EQ(FX_CharsetFlag::kChineseSimplified,
            FX_CharsetFlagForCharset(FX_Charset::kChineseSimplified));
  EXPECT_EQ(FX_CharsetFlag::kChineseTraditional,
            FX_CharsetFlagForCharset(FX_Charset::kChineseTraditional));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Greek,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Greek));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Turkish,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Turkish));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Vietnamese,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Vietnamese));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Hebrew,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Hebrew));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Arabic,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Arabic));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Baltic,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Baltic));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_Cyrillic,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_Cyrillic));
  EXPECT_EQ(FX_CharsetFlag::kThai, FX_CharsetFlagForCharset(FX_Charset::kThai));
  EXPECT_EQ(FX_CharsetFlag::kMSWin_EasternEuropean,
            FX_CharsetFlagForCharset(FX_Charset::kMSWin_EasternEuropean));
  EXPECT_EQ(FX_CharsetFlag::kOEM, FX_CharsetFlagForCharset(FX_Charset::kOEM));

  // Charsets without an explicit flag return kNone.
  EXPECT_EQ(FX_CharsetFlag::kNone,
            FX_CharsetFlagForCharset(FX_Charset::kDefault));
  EXPECT_EQ(FX_CharsetFlag::kNone, FX_CharsetFlagForCharset(FX_Charset::kUS));
  EXPECT_EQ(FX_CharsetFlag::kNone,
            FX_CharsetFlagForCharset(static_cast<FX_Charset>(253)));
}

TEST(FXCodePageTest, CharSetIsCJK) {
  EXPECT_TRUE(FX_CharSetIsCJK(FX_Charset::kShiftJIS));
  EXPECT_TRUE(FX_CharSetIsCJK(FX_Charset::kHangul));
  EXPECT_TRUE(FX_CharSetIsCJK(FX_Charset::kChineseSimplified));
  EXPECT_TRUE(FX_CharSetIsCJK(FX_Charset::kChineseTraditional));

  EXPECT_FALSE(FX_CharSetIsCJK(FX_Charset::kANSI));
  EXPECT_FALSE(FX_CharSetIsCJK(FX_Charset::kSymbol));
  EXPECT_FALSE(FX_CharSetIsCJK(FX_Charset::kMSWin_Cyrillic));
  EXPECT_FALSE(FX_CharSetIsCJK(FX_Charset::kThai));
  EXPECT_FALSE(FX_CharSetIsCJK(FX_Charset::kMSWin_EasternEuropean));
}

TEST(FXCodePageTest, CharacterConversion) {
  std::array<char, 4> mb_buf;
  EXPECT_EQ(4u, FX_WideCharToMultiByte(FX_CodePage::kDefANSI, L"Test", mb_buf));
  EXPECT_THAT(mb_buf, ElementsAre('T', 'e', 's', 't'));

  std::array<wchar_t, 4> wide_buf;
  EXPECT_EQ(4u,
            FX_MultiByteToWideChar(FX_CodePage::kDefANSI, "Test", wide_buf));
  EXPECT_THAT(wide_buf, ElementsAre(L'T', L'e', L's', L't'));
}
