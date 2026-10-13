// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxge/dib/cfx_dibbase.h"

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/span.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using ::testing::ElementsAre;

struct Input {
  CFX_Point src_top_left;
  CFX_Size src_size;
  CFX_Point dest_top_left;
  CFX_Size overlap_size;
};

struct Output {
  CFX_Point src_top_left;
  CFX_Point dest_top_left;
  CFX_Size overlap_size;
};

void RunOverlapRectTest(const CFX_DIBitmap* bitmap,
                        const Input& input,
                        const Output* expected_output) {
  // Initialize in-out parameters.
  int src_left = input.src_top_left.x;
  int src_top = input.src_top_left.y;
  int dest_left = input.dest_top_left.x;
  int dest_top = input.dest_top_left.y;
  int overlap_width = input.overlap_size.width;
  int overlap_height = input.overlap_size.height;

  bool success = bitmap->GetOverlapRect(
      dest_left, dest_top, overlap_width, overlap_height, input.src_size.width,
      input.src_size.height, src_left, src_top,
      /*pClipRgn=*/nullptr);
  if (success == !expected_output) {
    ADD_FAILURE();
    return;
  }

  if (expected_output) {
    EXPECT_EQ(expected_output->src_top_left.x, src_left);
    EXPECT_EQ(expected_output->src_top_left.y, src_top);
    EXPECT_EQ(expected_output->dest_top_left.x, dest_left);
    EXPECT_EQ(expected_output->dest_top_left.y, dest_top);
    EXPECT_EQ(expected_output->overlap_size.width, overlap_width);
    EXPECT_EQ(expected_output->overlap_size.height, overlap_height);
  }
}

}  // namespace

TEST(CFXDIBBaseTest, GetOverlapRectTrivialOverlap) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_TRUE(bitmap->Create(400, 300, FXDIB_Format::k1bppRgb));

  const Input kInput = {/*src_top_left=*/{0, 0}, /*src_size=*/{400, 300},
                        /*dest_top_left=*/{0, 0},
                        /*overlap_size=*/{400, 300}};
  const Output kExpectedOutput = {/*src_top_left=*/{0, 0},
                                  /*dest_top_left=*/{0, 0},
                                  /*overlap_size=*/{400, 300}};
  RunOverlapRectTest(bitmap.Get(), kInput, &kExpectedOutput);
}

TEST(CFXDIBBaseTest, GetOverlapRectOverlapNoLimit) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_TRUE(bitmap->Create(400, 300, FXDIB_Format::k1bppRgb));

  const Input kInput = {/*src_top_left=*/{35, 41}, /*src_size=*/{400, 300},
                        /*dest_top_left=*/{123, 137},
                        /*overlap_size=*/{200, 100}};
  const Output kExpectedOutput = {/*src_top_left=*/{35, 41},
                                  /*dest_top_left=*/{123, 137},
                                  /*overlap_size=*/{200, 100}};
  RunOverlapRectTest(bitmap.Get(), kInput, &kExpectedOutput);
}

TEST(CFXDIBBaseTest, GetOverlapRectOverlapLimitedBySource) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_TRUE(bitmap->Create(400, 300, FXDIB_Format::k1bppRgb));

  const Input kInput = {/*src_top_left=*/{141, 154}, /*src_size=*/{400, 300},
                        /*dest_top_left=*/{35, 41},
                        /*overlap_size=*/{270, 160}};
  const Output kExpectedOutput = {/*src_top_left=*/{141, 154},
                                  /*dest_top_left=*/{35, 41},
                                  /*overlap_size=*/{259, 146}};
  RunOverlapRectTest(bitmap.Get(), kInput, &kExpectedOutput);
}

TEST(CFXDIBBaseTest, GetOverlapRectOverlapLimitedByDestination) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_TRUE(bitmap->Create(400, 300, FXDIB_Format::k1bppRgb));

  const Input kInput = {/*src_top_left=*/{35, 41}, /*src_size=*/{400, 300},
                        /*dest_top_left=*/{123, 137},
                        /*overlap_size=*/{280, 170}};
  const Output kExpectedOutput = {/*src_top_left=*/{35, 41},
                                  /*dest_top_left=*/{123, 137},
                                  /*overlap_size=*/{277, 163}};
  RunOverlapRectTest(bitmap.Get(), kInput, &kExpectedOutput);
}

TEST(CFXDIBBaseTest, GetOverlapRectBadInputs) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  EXPECT_TRUE(bitmap->Create(400, 300, FXDIB_Format::k1bppRgb));

  const Input kEmptyInputs[] = {
      // Empty source rect.
      {/*src_top_left=*/{0, 0}, /*src_size=*/{0, 0},
       /*dest_top_left=*/{0, 0},
       /*overlap_size=*/{400, 300}},
      // Empty overlap size.
      {/*src_top_left=*/{0, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, 0},
       /*overlap_size=*/{0, 0}},
      // Source out of bounds on x-axis.
      {/*src_top_left=*/{-400, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, 0},
       /*overlap_size=*/{400, 300}},
  };
  for (const Input& input : kEmptyInputs) {
    RunOverlapRectTest(bitmap.Get(), input, /*expected_output=*/nullptr);
  }

  const Input kOutOfBoundInputs[] = {
      // Source out of bounds on x-axis.
      {/*src_top_left=*/{400, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, 0},
       /*overlap_size=*/{400, 300}},
      // Source out of bounds on y-axis.
      {/*src_top_left=*/{0, 300}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, 0},
       /*overlap_size=*/{400, 300}},
      // Source out of bounds on y-axis.
      {/*src_top_left=*/{0, -300}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, 0},
       /*overlap_size=*/{400, 300}},
      // Destination out of bounds on x-axis.
      {/*src_top_left=*/{0, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{-400, 0},
       /*overlap_size=*/{400, 300}},
      // Destination out of bounds on x-axis.
      {/*src_top_left=*/{0, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{400, 0},
       /*overlap_size=*/{400, 300}},
      // Destination out of bounds on y-axis.
      {/*src_top_left=*/{0, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, -300},
       /*overlap_size=*/{400, 300}},
      // Destination out of bounds on y-axis.
      {/*src_top_left=*/{0, 0}, /*src_size=*/{400, 300},
       /*dest_top_left=*/{0, 300},
       /*overlap_size=*/{400, 300}},
  };
  for (const Input& input : kOutOfBoundInputs) {
    RunOverlapRectTest(bitmap.Get(), input, /*expected_output=*/nullptr);
  }
}

TEST(CFXDIBBaseTest, SwapXY8Bpp) {
  auto src = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(src->Create(3, 2, FXDIB_Format::k8bppMask));
  auto row0 = src->GetWritableScanline(0);
  row0[0] = 10;
  row0[1] = 20;
  row0[2] = 30;
  auto row1 = src->GetWritableScanline(1);
  row1[0] = 40;
  row1[1] = 50;
  row1[2] = 60;

  auto ff = src->SwapXY(false, false);
  ASSERT_TRUE(ff);
  ASSERT_EQ(2, ff->GetWidth());
  ASSERT_EQ(3, ff->GetHeight());
  EXPECT_EQ(10u, ff->GetScanline(0)[0]);
  EXPECT_EQ(40u, ff->GetScanline(0)[1]);
  EXPECT_EQ(20u, ff->GetScanline(1)[0]);
  EXPECT_EQ(50u, ff->GetScanline(1)[1]);
  EXPECT_EQ(30u, ff->GetScanline(2)[0]);
  EXPECT_EQ(60u, ff->GetScanline(2)[1]);

  auto tf = src->SwapXY(true, false);
  ASSERT_TRUE(tf);
  ASSERT_EQ(2, tf->GetWidth());
  ASSERT_EQ(3, tf->GetHeight());
  EXPECT_EQ(40u, tf->GetScanline(0)[0]);
  EXPECT_EQ(10u, tf->GetScanline(0)[1]);
  EXPECT_EQ(50u, tf->GetScanline(1)[0]);
  EXPECT_EQ(20u, tf->GetScanline(1)[1]);
  EXPECT_EQ(60u, tf->GetScanline(2)[0]);
  EXPECT_EQ(30u, tf->GetScanline(2)[1]);

  auto ft = src->SwapXY(false, true);
  ASSERT_TRUE(ft);
  ASSERT_EQ(2, ft->GetWidth());
  ASSERT_EQ(3, ft->GetHeight());
  EXPECT_EQ(30u, ft->GetScanline(0)[0]);
  EXPECT_EQ(60u, ft->GetScanline(0)[1]);
  EXPECT_EQ(20u, ft->GetScanline(1)[0]);
  EXPECT_EQ(50u, ft->GetScanline(1)[1]);
  EXPECT_EQ(10u, ft->GetScanline(2)[0]);
  EXPECT_EQ(40u, ft->GetScanline(2)[1]);

  auto tt = src->SwapXY(true, true);
  ASSERT_TRUE(tt);
  ASSERT_EQ(2, tt->GetWidth());
  ASSERT_EQ(3, tt->GetHeight());
  EXPECT_EQ(60u, tt->GetScanline(0)[0]);
  EXPECT_EQ(30u, tt->GetScanline(0)[1]);
  EXPECT_EQ(50u, tt->GetScanline(1)[0]);
  EXPECT_EQ(20u, tt->GetScanline(1)[1]);
  EXPECT_EQ(40u, tt->GetScanline(2)[0]);
  EXPECT_EQ(10u, tt->GetScanline(2)[1]);
}

TEST(CFXDIBBaseTest, SwapXY32Bpp) {
  auto src = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(src->Create(3, 2, FXDIB_Format::kBgra));
  auto row0 = src->GetWritableScanlineAs<uint32_t>(0);
  row0[0] = 0x11223301u;
  row0[1] = 0x11223302u;
  row0[2] = 0x11223303u;
  auto row1 = src->GetWritableScanlineAs<uint32_t>(1);
  row1[0] = 0x44556604u;
  row1[1] = 0x44556605u;
  row1[2] = 0x44556606u;

  auto ff = src->SwapXY(false, false);
  ASSERT_TRUE(ff);
  ASSERT_EQ(2, ff->GetWidth());
  ASSERT_EQ(3, ff->GetHeight());
  EXPECT_EQ(0x11223301u, ff->GetScanlineAs<uint32_t>(0)[0]);
  EXPECT_EQ(0x44556604u, ff->GetScanlineAs<uint32_t>(0)[1]);
  EXPECT_EQ(0x11223302u, ff->GetScanlineAs<uint32_t>(1)[0]);
  EXPECT_EQ(0x44556605u, ff->GetScanlineAs<uint32_t>(1)[1]);
  EXPECT_EQ(0x11223303u, ff->GetScanlineAs<uint32_t>(2)[0]);
  EXPECT_EQ(0x44556606u, ff->GetScanlineAs<uint32_t>(2)[1]);

  auto tf = src->SwapXY(true, false);
  ASSERT_TRUE(tf);
  ASSERT_EQ(2, tf->GetWidth());
  ASSERT_EQ(3, tf->GetHeight());
  EXPECT_EQ(0x44556604u, tf->GetScanlineAs<uint32_t>(0)[0]);
  EXPECT_EQ(0x11223301u, tf->GetScanlineAs<uint32_t>(0)[1]);
  EXPECT_EQ(0x44556605u, tf->GetScanlineAs<uint32_t>(1)[0]);
  EXPECT_EQ(0x11223302u, tf->GetScanlineAs<uint32_t>(1)[1]);
  EXPECT_EQ(0x44556606u, tf->GetScanlineAs<uint32_t>(2)[0]);
  EXPECT_EQ(0x11223303u, tf->GetScanlineAs<uint32_t>(2)[1]);

  auto ft = src->SwapXY(false, true);
  ASSERT_TRUE(ft);
  ASSERT_EQ(2, ft->GetWidth());
  ASSERT_EQ(3, ft->GetHeight());
  EXPECT_EQ(0x11223303u, ft->GetScanlineAs<uint32_t>(0)[0]);
  EXPECT_EQ(0x44556606u, ft->GetScanlineAs<uint32_t>(0)[1]);
  EXPECT_EQ(0x11223302u, ft->GetScanlineAs<uint32_t>(1)[0]);
  EXPECT_EQ(0x44556605u, ft->GetScanlineAs<uint32_t>(1)[1]);
  EXPECT_EQ(0x11223301u, ft->GetScanlineAs<uint32_t>(2)[0]);
  EXPECT_EQ(0x44556604u, ft->GetScanlineAs<uint32_t>(2)[1]);

  auto tt = src->SwapXY(true, true);
  ASSERT_TRUE(tt);
  ASSERT_EQ(2, tt->GetWidth());
  ASSERT_EQ(3, tt->GetHeight());
  EXPECT_EQ(0x44556606u, tt->GetScanlineAs<uint32_t>(0)[0]);
  EXPECT_EQ(0x11223303u, tt->GetScanlineAs<uint32_t>(0)[1]);
  EXPECT_EQ(0x44556605u, tt->GetScanlineAs<uint32_t>(1)[0]);
  EXPECT_EQ(0x11223302u, tt->GetScanlineAs<uint32_t>(1)[1]);
  EXPECT_EQ(0x44556604u, tt->GetScanlineAs<uint32_t>(2)[0]);
  EXPECT_EQ(0x11223301u, tt->GetScanlineAs<uint32_t>(2)[1]);
}

TEST(CFXDIBBaseTest, CloneAlphaMask) {
  auto src = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(src->Create(2, 1, FXDIB_Format::kBgra));
  auto row = src->GetWritableScanlineAs<FX_BGRA_STRUCT<uint8_t>>(0);
  row[0] = {1, 2, 3, 0xaa};
  row[1] = {4, 5, 6, 0x55};

  auto mask = src->CloneAlphaMask();
  ASSERT_TRUE(mask);
  ASSERT_EQ(2, mask->GetWidth());
  ASSERT_EQ(1, mask->GetHeight());
  EXPECT_EQ(FXDIB_Format::k8bppMask, mask->GetFormat());
  EXPECT_EQ(0xaau, mask->GetScanline(0)[0]);
  EXPECT_EQ(0x55u, mask->GetScanline(0)[1]);
}

TEST(CFXDIBBaseTest, ConvertToBgrFromBgra) {
  auto src = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(src->Create(2, 1, FXDIB_Format::kBgra));
  auto row = src->GetWritableScanlineAs<FX_BGRA_STRUCT<uint8_t>>(0);
  row[0] = {10, 20, 30, 0x80};
  row[1] = {40, 50, 60, 0xff};

  auto bgr = src->ConvertTo(FXDIB_Format::kBgr);
  ASSERT_TRUE(bgr);
  ASSERT_EQ(2, bgr->GetWidth());
  ASSERT_EQ(1, bgr->GetHeight());
  EXPECT_EQ(FXDIB_Format::kBgr, bgr->GetFormat());
  auto bgr_row = bgr->GetScanlineAs<FX_BGR_STRUCT<uint8_t>>(0);
  EXPECT_EQ(10u, bgr_row[0].blue);
  EXPECT_EQ(20u, bgr_row[0].green);
  EXPECT_EQ(30u, bgr_row[0].red);
  EXPECT_EQ(40u, bgr_row[1].blue);
  EXPECT_EQ(50u, bgr_row[1].green);
  EXPECT_EQ(60u, bgr_row[1].red);
}

TEST(CFXDIBBaseTest, StretchToEmpty) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 3, FXDIB_Format::kBgra));

  EXPECT_FALSE(bitmap->StretchTo(0, 3, FXDIB_ResampleOptions(), nullptr));
  EXPECT_FALSE(bitmap->StretchTo(2, 0, FXDIB_ResampleOptions(), nullptr));

  constexpr FX_RECT kOutOfBoundsClip(10, 10, 20, 20);
  EXPECT_FALSE(
      bitmap->StretchTo(4, 6, FXDIB_ResampleOptions(), &kOutOfBoundsClip));
}

TEST(CFXDIBBaseTest, StretchToMaskFlipAndClip) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 2, FXDIB_Format::k8bppMask));
  auto src_scan0 = bitmap->GetWritableScanline(0);
  src_scan0[0] = 0x10;
  src_scan0[1] = 0x20;
  auto src_scan1 = bitmap->GetWritableScanline(1);
  src_scan1[0] = 0x30;
  src_scan1[1] = 0x40;

  constexpr FX_RECT kClip(1, 0, 3, 2);
  RetainPtr<CFX_DIBitmap> result =
      bitmap->StretchTo(-4, 2, FXDIB_ResampleOptions(), &kClip);
  ASSERT_TRUE(result);
  ASSERT_EQ(2, result->GetWidth());
  ASSERT_EQ(2, result->GetHeight());
  EXPECT_EQ(FXDIB_Format::k8bppMask, result->GetFormat());
  EXPECT_THAT(result->GetScanline(0).first(2u), ElementsAre(0x1c, 0x14));
  EXPECT_THAT(result->GetScanline(1).first(2u), ElementsAre(0x3c, 0x34));
}

TEST(CFXDIBBaseTest, StretchToGrayscale) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 2, FXDIB_Format::k8bppRgb));
  auto src_scan0 = bitmap->GetWritableScanline(0);
  src_scan0[0] = 0x10;
  src_scan0[1] = 0x20;
  auto src_scan1 = bitmap->GetWritableScanline(1);
  src_scan1[0] = 0x30;
  src_scan1[1] = 0x40;

  RetainPtr<CFX_DIBitmap> result =
      bitmap->StretchTo(4, -2, FXDIB_ResampleOptions(), nullptr);
  ASSERT_TRUE(result);
  ASSERT_EQ(4, result->GetWidth());
  ASSERT_EQ(2, result->GetHeight());
  EXPECT_EQ(FXDIB_Format::k8bppRgb, result->GetFormat());
  EXPECT_FALSE(result->HasPalette());
  EXPECT_THAT(result->GetScanline(0).first(4u),
              ElementsAre(0x30, 0x34, 0x3c, 0x40));
  EXPECT_THAT(result->GetScanline(1).first(4u),
              ElementsAre(0x10, 0x14, 0x1c, 0x20));
}

TEST(CFXDIBBaseTest, StretchToBgra) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 1, FXDIB_Format::kBgra));
  auto src_scan = bitmap->GetWritableScanlineAs<uint32_t>(0);
  src_scan[0] = 0xff112233;
  src_scan[1] = 0xff445566;

  RetainPtr<CFX_DIBitmap> result =
      bitmap->StretchTo(4, 2, FXDIB_ResampleOptions(), nullptr);
  ASSERT_TRUE(result);
  ASSERT_EQ(4, result->GetWidth());
  ASSERT_EQ(2, result->GetHeight());
  EXPECT_EQ(FXDIB_Format::kBgra, result->GetFormat());
  EXPECT_THAT(result->GetScanlineAs<uint32_t>(0),
              ElementsAre(0xff112233, 0xff112233, 0xff445566, 0xff445566));
  EXPECT_THAT(result->GetScanlineAs<uint32_t>(1),
              ElementsAre(0xff112233, 0xff112233, 0xff445566, 0xff445566));
}

TEST(CFXDIBBaseTest, TransformToEmpty) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 3, FXDIB_Format::kBgra));

  int left = -1;
  int top = -1;
  RetainPtr<CFX_DIBitmap> result =
      bitmap->TransformTo(CFX_Matrix(0, 0, 0, 0, 10, 20), &left, &top);
  EXPECT_FALSE(result);
  EXPECT_EQ(0, left);
  EXPECT_EQ(0, top);
}

TEST(CFXDIBBaseTest, TransformToMaskRotate90) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 3, FXDIB_Format::k8bppMask));
  auto src_scan0 = bitmap->GetWritableScanline(0);
  src_scan0[0] = 0x10;
  src_scan0[1] = 0x20;
  auto src_scan1 = bitmap->GetWritableScanline(1);
  src_scan1[0] = 0x30;
  src_scan1[1] = 0x40;
  auto src_scan2 = bitmap->GetWritableScanline(2);
  src_scan2[0] = 0x50;
  src_scan2[1] = 0x60;

  int left = 0;
  int top = 0;
  RetainPtr<CFX_DIBitmap> result =
      bitmap->TransformTo(CFX_Matrix(0, 2, 3, 0, -5, 7), &left, &top);
  ASSERT_TRUE(result);
  EXPECT_EQ(-5, left);
  EXPECT_EQ(7, top);
  ASSERT_EQ(3, result->GetWidth());
  ASSERT_EQ(2, result->GetHeight());
  EXPECT_EQ(FXDIB_Format::k8bppMask, result->GetFormat());
  EXPECT_THAT(result->GetScanline(0).first(3u), ElementsAre(0x50, 0x30, 0x10));
  EXPECT_THAT(result->GetScanline(1).first(3u), ElementsAre(0x60, 0x40, 0x20));
}

TEST(CFXDIBBaseTest, TransformToBgraScaleAndTranslate) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  ASSERT_TRUE(bitmap->Create(2, 1, FXDIB_Format::kBgra));
  auto src_scan = bitmap->GetWritableScanlineAs<uint32_t>(0);
  src_scan[0] = 0xff112233;
  src_scan[1] = 0xff445566;

  int left = 0;
  int top = 0;
  RetainPtr<CFX_DIBitmap> result =
      bitmap->TransformTo(CFX_Matrix(4, 0, 0, -2, 10, 22), &left, &top);
  ASSERT_TRUE(result);
  EXPECT_EQ(10, left);
  EXPECT_EQ(20, top);
  ASSERT_EQ(4, result->GetWidth());
  ASSERT_EQ(2, result->GetHeight());
  EXPECT_EQ(FXDIB_Format::kBgra, result->GetFormat());
  EXPECT_THAT(result->GetScanlineAs<uint32_t>(0),
              ElementsAre(0xff112233, 0xff112233, 0xff445566, 0xff445566));
  EXPECT_THAT(result->GetScanlineAs<uint32_t>(1),
              ElementsAre(0xff112233, 0xff112233, 0xff445566, 0xff445566));
}
