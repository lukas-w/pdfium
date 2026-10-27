// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/agg/cfx_agg_cliprgn.h"

#include <stdint.h>

#include <algorithm>
#include <utility>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

RetainPtr<CFX_DIBitmap> CreateMask(int width, int height, uint8_t fill_val) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!bitmap->Create(width, height, FXDIB_Format::k8bppMask)) {
    return nullptr;
  }
  std::ranges::fill(bitmap->GetWritableBuffer(), fill_val);
  return bitmap;
}

RetainPtr<CFX_DIBitmap> CreateGradientMask(int width, int height) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!bitmap->Create(width, height, FXDIB_Format::k8bppMask)) {
    return nullptr;
  }
  for (int row = 0; row < height; ++row) {
    auto scan = bitmap->GetWritableScanline(row);
    for (int col = 0; col < width; ++col) {
      // Give each pixel a unique value based on its row and column (for masks
      // up to 10x10) so offset bugs are caught, starting at 100 so multiplying
      // two masks and dividing by 255 does not round down to 0.
      scan[col] = static_cast<uint8_t>(100 + row * 10 + col);
    }
  }
  return bitmap;
}

}  // namespace

// Intersecting disjoint rects produces an empty clip box, and subsequent
// IntersectMask() calls on an empty clip leave GetMask() null.
TEST(CFX_AggClipRgnTest, EmptyClip) {
  CFX_AggClipRgn clip(10, 10);
  EXPECT_EQ(FX_RECT(0, 0, 10, 10), clip.GetBox());
  EXPECT_FALSE(clip.GetMask());

  clip.IntersectRect(FX_RECT(20, 20, 30, 30));
  EXPECT_TRUE(clip.GetBox().IsEmpty());
  EXPECT_FALSE(clip.GetMask());

  auto mask = CreateMask(5, 5, 255);
  clip.IntersectMask(0, 0, mask);
  EXPECT_TRUE(clip.GetBox().IsEmpty());
  EXPECT_FALSE(clip.GetMask());
}

// When no mask exists yet and the incoming mask lies completely within
// GetBox(), IntersectMask() adopts the bitmap directly without copying.
TEST(CFX_AggClipRgnTest, InitialMaskMatchingBox) {
  CFX_AggClipRgn clip(5, 5);
  auto mask = CreateMask(5, 5, 128);
  clip.IntersectMask(0, 0, mask);

  EXPECT_EQ(FX_RECT(0, 0, 5, 5), clip.GetBox());
  ASSERT_TRUE(clip.GetMask());
  EXPECT_EQ(mask, clip.GetMask());
  EXPECT_EQ(128, clip.GetMask()->GetScanline(0)[0]);
}

// When no mask exists yet and the incoming mask extends outside GetBox(),
// IntersectMask() allocates a new bitmap cropped to the intersection.
TEST(CFX_AggClipRgnTest, InitialMaskSubBox) {
  CFX_AggClipRgn clip(4, 4);
  clip.IntersectRect(FX_RECT(1, 2, 4, 4));
  auto mask = CreateGradientMask(4, 4);
  clip.IntersectMask(0, 0, mask);

  EXPECT_EQ(FX_RECT(1, 2, 4, 4), clip.GetBox());
  ASSERT_TRUE(clip.GetMask());
  EXPECT_NE(mask, clip.GetMask());
  EXPECT_EQ(3, clip.GetMask()->GetWidth());
  EXPECT_EQ(2, clip.GetMask()->GetHeight());
  for (int row = 0; row < 2; ++row) {
    auto actual_scan = clip.GetMask()->GetScanline(row);
    auto expected_scan = mask->GetScanline(row + 2);
    for (int col = 0; col < 3; ++col) {
      EXPECT_EQ(expected_scan[col + 1], actual_scan[col]);
    }
  }
}

// Intersecting two masks with identical bounds multiplies pixel values as
// (old_val * new_val) / 255.
TEST(CFX_AggClipRgnTest, MatchingBoxMaskIntersection) {
  CFX_AggClipRgn clip(4, 4);
  auto mask1 = CreateMask(4, 4, 200);
  clip.IntersectMask(0, 0, std::move(mask1));

  ASSERT_TRUE(clip.GetMask());
  EXPECT_EQ(200, clip.GetMask()->GetScanline(0)[0]);

  auto mask2 = CreateMask(4, 4, 100);
  clip.IntersectMask(0, 0, mask2);

  // 200 * 100 / 255 = 78.
  ASSERT_TRUE(clip.GetMask());
  for (int row = 0; row < 4; ++row) {
    auto scan = clip.GetMask()->GetScanline(row);
    for (int col = 0; col < 4; ++col) {
      EXPECT_EQ(78, scan[col]);
    }
  }
}

// Copying a CFX_AggClipRgn shares the underlying mask bitmap, so a subsequent
// IntersectMask() call allocates a new bitmap without mutating the copy.
TEST(CFX_AggClipRgnTest, SavedClipPreserved) {
  CFX_AggClipRgn clip(4, 4);
  auto mask1 = CreateMask(4, 4, 255);
  clip.IntersectMask(0, 0, mask1);

  // Copy clip (simulating SaveState).
  CFX_AggClipRgn saved_clip = clip;

  auto mask2 = CreateMask(4, 4, 128);
  clip.IntersectMask(0, 0, mask2);

  // The saved clip must not be modified.
  ASSERT_TRUE(saved_clip.GetMask());
  EXPECT_EQ(255, saved_clip.GetMask()->GetScanline(0)[0]);

  // The modified clip must have the multiplied result.
  ASSERT_TRUE(clip.GetMask());
  EXPECT_EQ(128, clip.GetMask()->GetScanline(0)[0]);
  EXPECT_NE(saved_clip.GetMask(), clip.GetMask());
}

// Intersecting partially overlapping gradient masks exercises non-zero row and
// column offsets into both the existing mask and the incoming mask.
TEST(CFX_AggClipRgnTest, ShrinkingMaskIntersection) {
  CFX_AggClipRgn clip(4, 4);
  auto mask1 = CreateGradientMask(3, 3);
  clip.IntersectMask(0, 1, mask1);

  // Overlaps (0, 1, 3, 4) with (1, 0, 4, 3) -> (1, 1, 3, 3).
  auto mask2 = CreateGradientMask(3, 3);
  clip.IntersectMask(1, 0, mask2);

  EXPECT_EQ(FX_RECT(1, 1, 3, 3), clip.GetBox());
  ASSERT_TRUE(clip.GetMask());
  EXPECT_EQ(2, clip.GetMask()->GetWidth());
  EXPECT_EQ(2, clip.GetMask()->GetHeight());

  // The intersection starts at (1, 1), which is offset by col=1 in `mask1` at
  // (0, 1) and by row=1 in `mask2` at (1, 0).
  for (int row = 0; row < 2; ++row) {
    auto scan = clip.GetMask()->GetScanline(row);
    auto scan1 = mask1->GetScanline(row);
    auto scan2 = mask2->GetScanline(row + 1);
    for (int col = 0; col < 2; ++col) {
      uint8_t expected = (scan1[col + 1] * scan2[col]) / 255;
      EXPECT_EQ(expected, scan[col]);
    }
  }
}
