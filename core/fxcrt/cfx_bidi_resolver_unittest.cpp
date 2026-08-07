// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/cfx_bidi_resolver.h"

#include <ostream>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

bool operator==(const CFX_BidiResolver::ResolvedRun& lhs,
                const CFX_BidiResolver::ResolvedRun& rhs) {
  return lhs.start == rhs.start && lhs.length == rhs.length &&
         lhs.is_rtl == rhs.is_rtl;
}

void PrintTo(const CFX_BidiResolver::ResolvedRun& run, std::ostream* os) {
  *os << "ResolvedRun{start=" << run.start << ", length=" << run.length
      << ", is_rtl=" << (run.is_rtl ? "true" : "false") << "}";
}

TEST(fxcrt, BidiResolverPureLTR) {
  auto resolver = CFX_BidiResolver::Create(
      u"abc", CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);
  EXPECT_THAT(resolver->GetVisualRunsForLine(0, 3),
              ElementsAre(CFX_BidiResolver::ResolvedRun{0, 3, false}));
}

TEST(fxcrt, BidiResolverPureRTL) {
  // Hebrew characters Aleph, Bet, Gimel (RTL characters).
  auto resolver = CFX_BidiResolver::Create(
      u"\x05D0\x05D1\x05D2",
      CFX_BidiResolver::ParagraphDirection::kRightToLeft);
  ASSERT_TRUE(resolver);
  EXPECT_THAT(resolver->GetVisualRunsForLine(0, 3),
              ElementsAre(CFX_BidiResolver::ResolvedRun{0, 3, true}));
}

TEST(fxcrt, BidiResolverMixed) {
  auto resolver = CFX_BidiResolver::Create(
      u"ab \x05D0\x05D1 cd",
      CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);
  // "ab " (LTR) -> "Aleph Bet" (RTL) -> " cd" (LTR)
  EXPECT_THAT(resolver->GetVisualRunsForLine(0, 8),
              ElementsAre(CFX_BidiResolver::ResolvedRun{0, 3, false},
                          CFX_BidiResolver::ResolvedRun{3, 2, true},
                          CFX_BidiResolver::ResolvedRun{5, 3, false}));
}

TEST(fxcrt, BidiResolverLineWrap) {
  auto resolver = CFX_BidiResolver::Create(
      u"abc def ghi", CFX_BidiResolver::ParagraphDirection::kLeftToRight);
  ASSERT_TRUE(resolver);
  // Get runs for the middle word "def "
  EXPECT_THAT(resolver->GetVisualRunsForLine(4, 4),
              ElementsAre(CFX_BidiResolver::ResolvedRun{4, 4, false}));
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
  // ICU typically substitutes invalid bytes or treats them as neutral.
  // Because the paragraph direction is LTR, these neutral characters
  // inherit the base direction and resolve to a single LTR run.
  EXPECT_THAT(resolver->GetVisualRunsForLine(0, 2),
              ElementsAre(CFX_BidiResolver::ResolvedRun{0, 2, false}));
}
