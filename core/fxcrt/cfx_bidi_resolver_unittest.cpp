// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_bidi_resolver.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(fxcrt, BidiResolverPureLTR) {
  auto resolver = CFX_BidiResolver::Create(
      u"abc", CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);
  auto runs = resolver->GetVisualRunsForLine(0, 3);
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(3, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);
}

TEST(fxcrt, BidiResolverPureRTL) {
  // Hebrew characters Aleph, Bet, Gimel (RTL characters).
  auto resolver = CFX_BidiResolver::Create(
      u"\x05D0\x05D1\x05D2",
      CFX_BidiResolver::ParagraphDirection::kRightToLeft);
  ASSERT_TRUE(resolver);
  auto runs = resolver->GetVisualRunsForLine(0, 3);
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(3, runs[0].length);
  EXPECT_TRUE(runs[0].is_rtl);
}

TEST(fxcrt, BidiResolverMixed) {
  auto resolver = CFX_BidiResolver::Create(
      u"ab \x05D0\x05D1 cd",
      CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);
  auto runs = resolver->GetVisualRunsForLine(0, 8);
  ASSERT_EQ(3u, runs.size());
  // "ab " (LTR) -> "Aleph Bet" (RTL) -> " cd" (LTR)
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(3, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);

  EXPECT_EQ(3, runs[1].start);
  EXPECT_EQ(2, runs[1].length);
  EXPECT_TRUE(runs[1].is_rtl);

  EXPECT_EQ(5, runs[2].start);
  EXPECT_EQ(3, runs[2].length);
  EXPECT_FALSE(runs[2].is_rtl);
}

TEST(fxcrt, BidiResolverLineWrap) {
  auto resolver = CFX_BidiResolver::Create(
      u"abc def ghi", CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);
  // Get runs for the middle word "def "
  auto runs = resolver->GetVisualRunsForLine(4, 4);
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(4, runs[0].start);
  EXPECT_EQ(4, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);
}

TEST(fxcrt, BidiResolverEmpty) {
  auto resolver = CFX_BidiResolver::Create(
      std::u16string(), CFX_BidiResolver::ParagraphDirection::kAuto);
  // Just proving it doesn't crash or leak memory.
  EXPECT_FALSE(resolver);
}

TEST(fxcrt, BidiResolverOutOfBounds) {
  auto resolver = CFX_BidiResolver::Create(
      u"abc", CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);

  // Negative start
  EXPECT_THAT(resolver->GetVisualRunsForLine(-1, 3), testing::IsEmpty());

  // Negative length
  EXPECT_THAT(resolver->GetVisualRunsForLine(0, -1), testing::IsEmpty());

  // Zero length
  EXPECT_THAT(resolver->GetVisualRunsForLine(0, 0), testing::IsEmpty());

  // Start + length is past the end
  EXPECT_THAT(resolver->GetVisualRunsForLine(1, 3), testing::IsEmpty());

  // Start is past the end
  EXPECT_THAT(resolver->GetVisualRunsForLine(4, 1), testing::IsEmpty());
}

TEST(fxcrt, BidiResolverInvalidUTF16) {
  // Pass an isolated trailing surrogate (invalid UTF-16 sequence)
  auto resolver = CFX_BidiResolver::Create(
      u"\xDC00\xDC00", CFX_BidiResolver::ParagraphDirection::kLeftToRight);

  ASSERT_TRUE(resolver);
  auto runs = resolver->GetVisualRunsForLine(0, 2);
  // ICU typically substitutes invalid bytes or treats them as neutral.
  // Because the paragraph direction is LTR, these neutral characters
  // inherit the base direction and resolve to a single LTR run.
  ASSERT_EQ(1u, runs.size());
  EXPECT_EQ(0, runs[0].start);
  EXPECT_EQ(2, runs[0].length);
  EXPECT_FALSE(runs[0].is_rtl);
}
