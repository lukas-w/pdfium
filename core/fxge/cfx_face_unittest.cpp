// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/cfx_face.h"

#include <memory>

#include "core/fxcrt/cfx_read_only_span_stream.h"
#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/cfx_glyphbitmap.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/cfx_substfont.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/fontdata/chromefontdata/chromefontdata.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/fx_fontencoding.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(PDF_ENABLE_FONTATIONS)
TEST(CFXFaceTest, BuiltInGenericFontScaling) {
  if (!CFX_GEModule::IsFontations()) {
    GTEST_SKIP() << "Not a fontations-based renderer";
  }

  auto stream =
      pdfium::MakeRetain<CFX_ReadOnlySpanStream>(kFoxitSansMMFontData);
  auto face = CFX_Face::New(nullptr, stream, 0);
  ASSERT_TRUE(face);

  CFX_SubstFont subst_font;
  subst_font.SetIsBuiltInGenericFont();

  EXPECT_TRUE(face->SelectCharMap(fxge::FontEncoding::kAdobeCustom));
  int gid_x = face->GetCharIndex(120);
  ASSERT_EQ(gid_x, 103);

  int unscaled_width = face->GetGlyphWidth(gid_x, /*dest_width=*/0,
                                           /*weight=*/0, &subst_font);
  EXPECT_EQ(unscaled_width, 530);

  int target_dest_width = unscaled_width * 2;
  int scaled_width = face->GetGlyphWidth(gid_x, target_dest_width,
                                         /*weight=*/0, &subst_font);
  EXPECT_EQ(scaled_width, target_dest_width);

  auto unscaled_path = face->LoadGlyphPath(gid_x, /*dest_width=*/0,
                                           /*is_vertical=*/false, &subst_font);
  ASSERT_TRUE(unscaled_path);

  auto scaled_path = face->LoadGlyphPath(gid_x, target_dest_width,
                                         /*is_vertical=*/false, &subst_font);
  ASSERT_TRUE(scaled_path);

  float unscaled_path_width = unscaled_path->GetBoundingBox().Width();
  float scaled_path_width = scaled_path->GetBoundingBox().Width();
  EXPECT_NEAR(scaled_path_width, unscaled_path_width * 2.0f, 0.5f);

#if defined(PDF_ENABLE_FREETYPE)
  constexpr CFX_Matrix kMatrix(12.0f, 0, 0, 12.0f, 0, 0);
  auto unscaled_bitmap = face->RenderGlyph(
      gid_x, /*is_cid_font=*/false, /*is_vertical=*/false, kMatrix,
      /*dest_width=*/0, FontAntiAliasingMode::kNormal, &subst_font);
  ASSERT_TRUE(unscaled_bitmap);
  ASSERT_TRUE(unscaled_bitmap->GetBitmap());

  auto scaled_bitmap = face->RenderGlyph(
      gid_x, /*is_cid_font=*/false, /*is_vertical=*/false, kMatrix,
      target_dest_width, FontAntiAliasingMode::kNormal, &subst_font);
  ASSERT_TRUE(scaled_bitmap);
  ASSERT_TRUE(scaled_bitmap->GetBitmap());

  EXPECT_GT(scaled_bitmap->GetBitmap()->GetWidth(),
            unscaled_bitmap->GetBitmap()->GetWidth());
#endif  // defined(PDF_ENABLE_FREETYPE)
}
#endif  // defined(PDF_ENABLE_FONTATIONS)
