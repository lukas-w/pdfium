// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/fx_unicode.h"

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace pdfium::unicode {

#if defined(WCHAR_T_IS_32_BIT)
TEST(FXUnicodeTest, GetMirrorCharSupplementary) {
  // TODO(crbug.com/555940408): Should be L'\U00010000'.
  EXPECT_EQ(L')', GetMirrorChar(L'\U00010000'));
  // TODO(crbug.com/555940408): Should be L'🎨'.
  EXPECT_EQ(L')', GetMirrorChar(L'🎨'));
}
#endif  // defined(WCHAR_T_IS_32_BIT)

}  // namespace pdfium::unicode
