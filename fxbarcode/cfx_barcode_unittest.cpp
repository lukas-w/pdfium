// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxbarcode/cfx_barcode.h"

#include <memory>
#include <string>
#include <utility>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/bitmap_saver.h"
#include "testing/utils/hash.h"

class BarcodeTest : public testing::Test {
 public:
  void SetUp() override {
    BC_Library_Init();

    auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
    if (bitmap->Create(640, 480, FXDIB_Format::kBgrx)) {
      bitmap_ = bitmap;
    }
    ASSERT_TRUE(bitmap_);
    auto device = CFX_RenderDevice::CreateForBitmap(bitmap_);
    ASSERT_TRUE(device);
    device_ = std::move(device);
  }

  void TearDown() override {
    bitmap_.Reset();
    device_.reset();
    barcode_.reset();
    BC_Library_Destroy();
  }

  CFX_Barcode* barcode() const { return barcode_.get(); }

  void Create(BC_TYPE type) {
    barcode_ = std::make_unique<CFX_Barcode>(type);
    barcode_->SetHeight(298);
    barcode_->SetWidth(418);
  }

  bool RenderDevice() { return barcode_->RenderDevice(device_.get(), matrix_); }

  std::string BitmapChecksum() {
    return GenerateMD5Base16(bitmap_->GetBuffer());
  }

  // Manually insert calls to this as needed for debugging.
  void SaveBitmap(const std::string& filename) {
    BitmapSaver::WriteBitmapToPng(bitmap_.Get(), filename);
  }

 protected:
  CFX_Matrix matrix_;
  std::unique_ptr<CFX_Barcode> barcode_;
  std::unique_ptr<CFX_RenderDevice> device_;
  RetainPtr<CFX_DIBitmap> bitmap_;
};

TEST_F(BarcodeTest, Code39) {
  Create(BC_TYPE::kCode39);
  EXPECT_TRUE(barcode()->Encode(L"CLAMS"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "f1a566abd7813ad9e2593e873b870653";
#else
  static constexpr char kExpectedChecksum[] =
      "cd4cd3f36da38ff58d9f621827018903";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, CodaBar) {
  Create(BC_TYPE::kCodabar);
  EXPECT_TRUE(barcode()->Encode(L"$123-456"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "5f229b391517f1140b95770a57e31990";
#else
  static constexpr char kExpectedChecksum[] =
      "5fad4fc19f099001a0fe83c89430c977";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, CodaBarLetters) {
  Create(BC_TYPE::kCodabar);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, Code128) {
  Create(BC_TYPE::kCode128);
  EXPECT_TRUE(barcode()->Encode(L"Clams"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "a33a5f68755842561064e4f57fcab06b";
#else
  static constexpr char kExpectedChecksum[] =
      "6351f0f6e997050e4658bbb4777aef74";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, Code128B) {
  Create(BC_TYPE::kCode128B);
  EXPECT_TRUE(barcode()->Encode(L"Clams"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "a33a5f68755842561064e4f57fcab06b";
#else
  static constexpr char kExpectedChecksum[] =
      "6351f0f6e997050e4658bbb4777aef74";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, Code128C) {
  Create(BC_TYPE::kCode128C);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "4e599185bfb9625664756e5009468c8e";
#else
  static constexpr char kExpectedChecksum[] =
      "fba730a807ba6363f9bd2bc7f8c56d1f";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, Code128CLetters) {
  Create(BC_TYPE::kCode128C);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "8276a6082d77c3db7abd8bc8850e8467";
#else
  static constexpr char kExpectedChecksum[] =
      "6284ec8503d5a948c9518108da33cdd3";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, Ean8) {
  Create(BC_TYPE::kEAN8);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "a8969be3f36f6f4b155d58f8171d7827";
#else
  static constexpr char kExpectedChecksum[] =
      "aff88491ac46ca6217d780d185300cde";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, Ean8Letters) {
  Create(BC_TYPE::kEAN8);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, UPCA) {
  Create(BC_TYPE::kUPCA);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "d9004b9e6feef54c91a6403a718bbe06";
#else
  static constexpr char kExpectedChecksum[] =
      "fe26a5714cff7ffe3f9b02183efc435b";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, UPCALetters) {
  Create(BC_TYPE::kUPCA);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, Ean13) {
  Create(BC_TYPE::kEAN13);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "efc7a49c967901efa6f63c59a10fedab";
#else
  static constexpr char kExpectedChecksum[] =
      "72d2190b98d635c32834bf67552e561e";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, Ean13Letters) {
  Create(BC_TYPE::kEAN13);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, Pdf417) {
  Create(BC_TYPE::kPDF417);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "63ee1988d3de512df2c3037d21bdf917";
#else
  static constexpr char kExpectedChecksum[] =
      "191e35d11613901b7d5d51033689aa89";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, DataMatrix) {
  Create(BC_TYPE::kDataMatrix);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "de33aa8fe06e24b6ab191643da8dd336";
#else
  static constexpr char kExpectedChecksum[] =
      "5e5cd9a680b86fcd4ffd53ed36e3c980";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}

TEST_F(BarcodeTest, QrCode) {
  Create(BC_TYPE::kQRCode);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
#if defined(PDF_USE_SKIA)
  static constexpr char kExpectedChecksum[] =
      "74d2e6de981088e69740a970b567efc0";
#else
  static constexpr char kExpectedChecksum[] =
      "4751c6e0f67749fabe24f787128decee";
#endif
  EXPECT_EQ(kExpectedChecksum, BitmapChecksum());
}
