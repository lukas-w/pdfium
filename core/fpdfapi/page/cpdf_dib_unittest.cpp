// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfapi/page/cpdf_dib.h"

#include <stdint.h>

#include <utility>

#include "core/fpdfapi/page/test_with_page_module.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_test_document.h"
#include "core/fxcrt/data_vector.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using CPDFDIBTest = TestWithPageModule;

// The width every test uses. Two pixels is enough to catch a wrong stride.
constexpr int kWidth = 2;

// Builds an uncompressed 2x1 DeviceRGB image XObject with the given sample
// data and loads it as a CPDF_DIB. The DIB stores 24bpp images as kBgr, so
// the scanline it returns is the default-decode translation of `data`.
RetainPtr<CPDF_DIB> LoadDeviceRgbImage(CPDF_Document* doc,
                                       int bits_per_component,
                                       DataVector<uint8_t> data) {
  auto dict = pdfium::MakeRetain<CPDF_Dictionary>();
  dict->SetNewFor<CPDF_Name>("Type", "XObject");
  dict->SetNewFor<CPDF_Name>("Subtype", "Image");
  dict->SetNewFor<CPDF_Number>("Width", kWidth);
  dict->SetNewFor<CPDF_Number>("Height", 1);
  dict->SetNewFor<CPDF_Number>("BitsPerComponent", bits_per_component);
  dict->SetNewFor<CPDF_Name>("ColorSpace", "DeviceRGB");
  auto stream =
      pdfium::MakeRetain<CPDF_Stream>(std::move(data), std::move(dict));
  auto dib = pdfium::MakeRetain<CPDF_DIB>(doc, std::move(stream));
  if (!dib->Load()) {
    return nullptr;
  }
  return dib;
}

}  // namespace

// Fewer than 8 bits per component: each component is read out of the bit
// stream and scaled to 8 bits.
TEST_F(CPDFDIBTest, DefaultDecode4BitsPerComponent) {
  CPDF_TestDocument doc;
  // Two pixels of 3 components at 4 bits each: R=0x0 G=0x8 B=0xf, then
  // R=0xf G=0x0 B=0x8.
  DataVector<uint8_t> data = {0x08, 0xff, 0x08};
  RetainPtr<CPDF_DIB> dib =
      LoadDeviceRgbImage(&doc, /*bits_per_component=*/4, std::move(data));
  ASSERT_TRUE(dib);
  // Each 4-bit sample scaled by 255 / 15, stored blue, green, red.
  EXPECT_THAT(dib->GetScanline(0),
              testing::ElementsAre(0xff, 0x88, 0x00, 0x88, 0x00, 0xff));
}

// 8 bits per component is a plain RGB-to-BGR swap.
TEST_F(CPDFDIBTest, DefaultDecode8BitsPerComponent) {
  CPDF_TestDocument doc;
  DataVector<uint8_t> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  RetainPtr<CPDF_DIB> dib =
      LoadDeviceRgbImage(&doc, /*bits_per_component=*/8, std::move(data));
  ASSERT_TRUE(dib);
  EXPECT_THAT(dib->GetScanline(0),
              testing::ElementsAre(0x33, 0x22, 0x11, 0x66, 0x55, 0x44));
}

// 16 bits per component keeps the high byte of each big-endian sample.
TEST_F(CPDFDIBTest, DefaultDecode16BitsPerComponent) {
  CPDF_TestDocument doc;
  // Two pixels, samples big-endian: R=0x1122 G=0x3344 B=0x5566, then
  // R=0x7788 G=0x99aa B=0xbbcc.
  DataVector<uint8_t> data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66,
                              0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc};
  RetainPtr<CPDF_DIB> dib =
      LoadDeviceRgbImage(&doc, /*bits_per_component=*/16, std::move(data));
  ASSERT_TRUE(dib);
  EXPECT_THAT(dib->GetScanline(0),
              testing::ElementsAre(0x55, 0x33, 0x11, 0xbb, 0x99, 0x77));
}
