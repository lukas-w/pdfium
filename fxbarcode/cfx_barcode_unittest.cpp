// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fxbarcode/cfx_barcode.h"

#include <memory>
#include <string>
#include <utility>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/display_list_device_driver.h"
#include "testing/gtest/include/gtest/gtest.h"

class BarcodeTest : public testing::Test {
 public:
  void SetUp() override {
    BC_Library_Init();

    auto driver = std::make_unique<DisplayListDeviceDriver>(640, 480);
    driver_ = driver.get();
    device_ = CFX_RenderDevice::CreateWithDriver(std::move(driver));
    ASSERT_TRUE(device_);
  }

  void TearDown() override {
    driver_ = nullptr;
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

  const ByteString& display_list() const { return driver_->display_list(); }

 protected:
  CFX_Matrix matrix_;
  std::unique_ptr<CFX_Barcode> barcode_;
  std::unique_ptr<CFX_RenderDevice> device_;
  DisplayListDeviceDriver* driver_ = nullptr;
};

TEST_F(BarcodeTest, Code39) {
  Create(BC_TYPE::kCode39);
  EXPECT_TRUE(barcode()->Encode(L"CLAMS"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,4,298] color=0xff000000\n"
      "FillRect [15,0,19,298] color=0xff000000\n"
      "FillRect [22,0,26,298] color=0xff000000\n"
      "FillRect [26,0,30,298] color=0xff000000\n"
      "FillRect [30,0,34,298] color=0xff000000\n"
      "FillRect [38,0,42,298] color=0xff000000\n"
      "FillRect [41,0,45,298] color=0xff000000\n"
      "FillRect [45,0,49,298] color=0xff000000\n"
      "FillRect [53,0,57,298] color=0xff000000\n"
      "FillRect [60,0,64,298] color=0xff000000\n"
      "FillRect [64,0,68,298] color=0xff000000\n"
      "FillRect [68,0,72,298] color=0xff000000\n"
      "FillRect [75,0,79,298] color=0xff000000\n"
      "FillRect [79,0,83,298] color=0xff000000\n"
      "FillRect [83,0,87,298] color=0xff000000\n"
      "FillRect [90,0,94,298] color=0xff000000\n"
      "FillRect [105,0,109,298] color=0xff000000\n"
      "FillRect [113,0,117,298] color=0xff000000\n"
      "FillRect [120,0,124,298] color=0xff000000\n"
      "FillRect [128,0,132,298] color=0xff000000\n"
      "FillRect [132,0,136,298] color=0xff000000\n"
      "FillRect [135,0,139,298] color=0xff000000\n"
      "FillRect [143,0,147,298] color=0xff000000\n"
      "FillRect [151,0,155,298] color=0xff000000\n"
      "FillRect [166,0,170,298] color=0xff000000\n"
      "FillRect [169,0,173,298] color=0xff000000\n"
      "FillRect [173,0,177,298] color=0xff000000\n"
      "FillRect [181,0,185,298] color=0xff000000\n"
      "FillRect [184,0,188,298] color=0xff000000\n"
      "FillRect [188,0,192,298] color=0xff000000\n"
      "FillRect [196,0,200,298] color=0xff000000\n"
      "FillRect [203,0,207,298] color=0xff000000\n"
      "FillRect [218,0,222,298] color=0xff000000\n"
      "FillRect [226,0,230,298] color=0xff000000\n"
      "FillRect [230,0,234,298] color=0xff000000\n"
      "FillRect [233,0,237,298] color=0xff000000\n"
      "FillRect [241,0,245,298] color=0xff000000\n"
      "FillRect [245,0,249,298] color=0xff000000\n"
      "FillRect [248,0,252,298] color=0xff000000\n"
      "FillRect [256,0,260,298] color=0xff000000\n"
      "FillRect [260,0,264,298] color=0xff000000\n"
      "FillRect [263,0,267,298] color=0xff000000\n"
      "FillRect [271,0,275,298] color=0xff000000\n"
      "FillRect [279,0,283,298] color=0xff000000\n"
      "FillRect [294,0,298,298] color=0xff000000\n"
      "FillRect [301,0,305,298] color=0xff000000\n"
      "FillRect [309,0,313,298] color=0xff000000\n"
      "FillRect [312,0,316,298] color=0xff000000\n"
      "FillRect [316,0,320,298] color=0xff000000\n"
      "FillRect [324,0,328,298] color=0xff000000\n"
      "FillRect [331,0,335,298] color=0xff000000\n"
      "FillRect [335,0,339,298] color=0xff000000\n"
      "FillRect [339,0,343,298] color=0xff000000\n"
      "FillRect [354,0,358,298] color=0xff000000\n"
      "FillRect [361,0,365,298] color=0xff000000\n"
      "FillRect [376,0,380,298] color=0xff000000\n"
      "FillRect [384,0,388,298] color=0xff000000\n"
      "FillRect [388,0,392,298] color=0xff000000\n"
      "FillRect [392,0,396,298] color=0xff000000\n"
      "FillRect [399,0,403,298] color=0xff000000\n"
      "FillRect [403,0,407,298] color=0xff000000\n"
      "FillRect [407,0,411,298] color=0xff000000\n"
      "FillRect [414,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, CodaBar) {
  Create(BC_TYPE::kCodabar);
  EXPECT_TRUE(barcode()->Encode(L"$123-456"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,5,298] color=0xff000000\n"
      "FillRect [8,0,13,298] color=0xff000000\n"
      "FillRect [12,0,17,298] color=0xff000000\n"
      "FillRect [24,0,29,298] color=0xff000000\n"
      "FillRect [37,0,42,298] color=0xff000000\n"
      "FillRect [45,0,50,298] color=0xff000000\n"
      "FillRect [53,0,58,298] color=0xff000000\n"
      "FillRect [58,0,63,298] color=0xff000000\n"
      "FillRect [70,0,75,298] color=0xff000000\n"
      "FillRect [78,0,83,298] color=0xff000000\n"
      "FillRect [86,0,91,298] color=0xff000000\n"
      "FillRect [95,0,100,298] color=0xff000000\n"
      "FillRect [103,0,108,298] color=0xff000000\n"
      "FillRect [107,0,112,298] color=0xff000000\n"
      "FillRect [120,0,125,298] color=0xff000000\n"
      "FillRect [128,0,133,298] color=0xff000000\n"
      "FillRect [136,0,141,298] color=0xff000000\n"
      "FillRect [149,0,154,298] color=0xff000000\n"
      "FillRect [157,0,162,298] color=0xff000000\n"
      "FillRect [161,0,166,298] color=0xff000000\n"
      "FillRect [169,0,174,298] color=0xff000000\n"
      "FillRect [173,0,178,298] color=0xff000000\n"
      "FillRect [186,0,191,298] color=0xff000000\n"
      "FillRect [194,0,199,298] color=0xff000000\n"
      "FillRect [202,0,207,298] color=0xff000000\n"
      "FillRect [211,0,216,298] color=0xff000000\n"
      "FillRect [219,0,224,298] color=0xff000000\n"
      "FillRect [231,0,236,298] color=0xff000000\n"
      "FillRect [235,0,240,298] color=0xff000000\n"
      "FillRect [244,0,249,298] color=0xff000000\n"
      "FillRect [252,0,257,298] color=0xff000000\n"
      "FillRect [260,0,265,298] color=0xff000000\n"
      "FillRect [264,0,269,298] color=0xff000000\n"
      "FillRect [273,0,278,298] color=0xff000000\n"
      "FillRect [285,0,290,298] color=0xff000000\n"
      "FillRect [293,0,298,298] color=0xff000000\n"
      "FillRect [298,0,303,298] color=0xff000000\n"
      "FillRect [306,0,311,298] color=0xff000000\n"
      "FillRect [314,0,319,298] color=0xff000000\n"
      "FillRect [327,0,332,298] color=0xff000000\n"
      "FillRect [335,0,340,298] color=0xff000000\n"
      "FillRect [347,0,352,298] color=0xff000000\n"
      "FillRect [355,0,360,298] color=0xff000000\n"
      "FillRect [364,0,369,298] color=0xff000000\n"
      "FillRect [368,0,373,298] color=0xff000000\n"
      "FillRect [376,0,381,298] color=0xff000000\n"
      "FillRect [389,0,394,298] color=0xff000000\n"
      "FillRect [401,0,406,298] color=0xff000000\n"
      "FillRect [409,0,414,298] color=0xff000000\n"
      "FillRect [413,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, CodaBarLetters) {
  Create(BC_TYPE::kCodabar);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, Code128) {
  Create(BC_TYPE::kCode128);
  EXPECT_TRUE(barcode()->Encode(L"Clams"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,5,298] color=0xff000000\n"
      "FillRect [4,0,9,298] color=0xff000000\n"
      "FillRect [14,0,19,298] color=0xff000000\n"
      "FillRect [28,0,33,298] color=0xff000000\n"
      "FillRect [51,0,56,298] color=0xff000000\n"
      "FillRect [69,0,74,298] color=0xff000000\n"
      "FillRect [88,0,93,298] color=0xff000000\n"
      "FillRect [93,0,98,298] color=0xff000000\n"
      "FillRect [102,0,107,298] color=0xff000000\n"
      "FillRect [107,0,112,298] color=0xff000000\n"
      "FillRect [121,0,126,298] color=0xff000000\n"
      "FillRect [130,0,135,298] color=0xff000000\n"
      "FillRect [153,0,158,298] color=0xff000000\n"
      "FillRect [167,0,172,298] color=0xff000000\n"
      "FillRect [176,0,181,298] color=0xff000000\n"
      "FillRect [181,0,186,298] color=0xff000000\n"
      "FillRect [204,0,209,298] color=0xff000000\n"
      "FillRect [209,0,214,298] color=0xff000000\n"
      "FillRect [213,0,218,298] color=0xff000000\n"
      "FillRect [218,0,223,298] color=0xff000000\n"
      "FillRect [227,0,232,298] color=0xff000000\n"
      "FillRect [232,0,237,298] color=0xff000000\n"
      "FillRect [237,0,242,298] color=0xff000000\n"
      "FillRect [246,0,251,298] color=0xff000000\n"
      "FillRect [255,0,260,298] color=0xff000000\n"
      "FillRect [265,0,270,298] color=0xff000000\n"
      "FillRect [269,0,274,298] color=0xff000000\n"
      "FillRect [274,0,279,298] color=0xff000000\n"
      "FillRect [278,0,283,298] color=0xff000000\n"
      "FillRect [292,0,297,298] color=0xff000000\n"
      "FillRect [306,0,311,298] color=0xff000000\n"
      "FillRect [311,0,316,298] color=0xff000000\n"
      "FillRect [325,0,330,298] color=0xff000000\n"
      "FillRect [334,0,339,298] color=0xff000000\n"
      "FillRect [357,0,362,298] color=0xff000000\n"
      "FillRect [362,0,367,298] color=0xff000000\n"
      "FillRect [381,0,386,298] color=0xff000000\n"
      "FillRect [385,0,390,298] color=0xff000000\n"
      "FillRect [390,0,395,298] color=0xff000000\n"
      "FillRect [399,0,404,298] color=0xff000000\n"
      "FillRect [409,0,414,298] color=0xff000000\n"
      "FillRect [413,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, Code128B) {
  Create(BC_TYPE::kCode128B);
  EXPECT_TRUE(barcode()->Encode(L"Clams"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,5,298] color=0xff000000\n"
      "FillRect [4,0,9,298] color=0xff000000\n"
      "FillRect [14,0,19,298] color=0xff000000\n"
      "FillRect [28,0,33,298] color=0xff000000\n"
      "FillRect [51,0,56,298] color=0xff000000\n"
      "FillRect [69,0,74,298] color=0xff000000\n"
      "FillRect [88,0,93,298] color=0xff000000\n"
      "FillRect [93,0,98,298] color=0xff000000\n"
      "FillRect [102,0,107,298] color=0xff000000\n"
      "FillRect [107,0,112,298] color=0xff000000\n"
      "FillRect [121,0,126,298] color=0xff000000\n"
      "FillRect [130,0,135,298] color=0xff000000\n"
      "FillRect [153,0,158,298] color=0xff000000\n"
      "FillRect [167,0,172,298] color=0xff000000\n"
      "FillRect [176,0,181,298] color=0xff000000\n"
      "FillRect [181,0,186,298] color=0xff000000\n"
      "FillRect [204,0,209,298] color=0xff000000\n"
      "FillRect [209,0,214,298] color=0xff000000\n"
      "FillRect [213,0,218,298] color=0xff000000\n"
      "FillRect [218,0,223,298] color=0xff000000\n"
      "FillRect [227,0,232,298] color=0xff000000\n"
      "FillRect [232,0,237,298] color=0xff000000\n"
      "FillRect [237,0,242,298] color=0xff000000\n"
      "FillRect [246,0,251,298] color=0xff000000\n"
      "FillRect [255,0,260,298] color=0xff000000\n"
      "FillRect [265,0,270,298] color=0xff000000\n"
      "FillRect [269,0,274,298] color=0xff000000\n"
      "FillRect [274,0,279,298] color=0xff000000\n"
      "FillRect [278,0,283,298] color=0xff000000\n"
      "FillRect [292,0,297,298] color=0xff000000\n"
      "FillRect [306,0,311,298] color=0xff000000\n"
      "FillRect [311,0,316,298] color=0xff000000\n"
      "FillRect [325,0,330,298] color=0xff000000\n"
      "FillRect [334,0,339,298] color=0xff000000\n"
      "FillRect [357,0,362,298] color=0xff000000\n"
      "FillRect [362,0,367,298] color=0xff000000\n"
      "FillRect [381,0,386,298] color=0xff000000\n"
      "FillRect [385,0,390,298] color=0xff000000\n"
      "FillRect [390,0,395,298] color=0xff000000\n"
      "FillRect [399,0,404,298] color=0xff000000\n"
      "FillRect [409,0,414,298] color=0xff000000\n"
      "FillRect [413,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, Code128C) {
  Create(BC_TYPE::kCode128C);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,7,298] color=0xff000000\n"
      "FillRect [6,0,13,298] color=0xff000000\n"
      "FillRect [18,0,25,298] color=0xff000000\n"
      "FillRect [36,0,43,298] color=0xff000000\n"
      "FillRect [43,0,50,298] color=0xff000000\n"
      "FillRect [49,0,56,298] color=0xff000000\n"
      "FillRect [67,0,74,298] color=0xff000000\n"
      "FillRect [79,0,86,298] color=0xff000000\n"
      "FillRect [86,0,93,298] color=0xff000000\n"
      "FillRect [104,0,111,298] color=0xff000000\n"
      "FillRect [110,0,117,298] color=0xff000000\n"
      "FillRect [116,0,123,298] color=0xff000000\n"
      "FillRect [135,0,142,298] color=0xff000000\n"
      "FillRect [159,0,166,298] color=0xff000000\n"
      "FillRect [172,0,179,298] color=0xff000000\n"
      "FillRect [178,0,185,298] color=0xff000000\n"
      "FillRect [202,0,209,298] color=0xff000000\n"
      "FillRect [209,0,216,298] color=0xff000000\n"
      "FillRect [215,0,222,298] color=0xff000000\n"
      "FillRect [239,0,246,298] color=0xff000000\n"
      "FillRect [252,0,259,298] color=0xff000000\n"
      "FillRect [258,0,265,298] color=0xff000000\n"
      "FillRect [270,0,277,298] color=0xff000000\n"
      "FillRect [295,0,302,298] color=0xff000000\n"
      "FillRect [301,0,308,298] color=0xff000000\n"
      "FillRect [313,0,320,298] color=0xff000000\n"
      "FillRect [319,0,326,298] color=0xff000000\n"
      "FillRect [325,0,332,298] color=0xff000000\n"
      "FillRect [338,0,345,298] color=0xff000000\n"
      "FillRect [344,0,351,298] color=0xff000000\n"
      "FillRect [368,0,375,298] color=0xff000000\n"
      "FillRect [375,0,382,298] color=0xff000000\n"
      "FillRect [381,0,388,298] color=0xff000000\n"
      "FillRect [393,0,400,298] color=0xff000000\n"
      "FillRect [405,0,412,298] color=0xff000000\n"
      "FillRect [411,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, Code128CLetters) {
  Create(BC_TYPE::kCode128C);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,7,298] color=0xff000000\n"
      "FillRect [6,0,13,298] color=0xff000000\n"
      "FillRect [18,0,25,298] color=0xff000000\n"
      "FillRect [36,0,43,298] color=0xff000000\n"
      "FillRect [43,0,50,298] color=0xff000000\n"
      "FillRect [49,0,56,298] color=0xff000000\n"
      "FillRect [67,0,74,298] color=0xff000000\n"
      "FillRect [79,0,86,298] color=0xff000000\n"
      "FillRect [86,0,93,298] color=0xff000000\n"
      "FillRect [92,0,99,298] color=0xff000000\n"
      "FillRect [104,0,111,298] color=0xff000000\n"
      "FillRect [110,0,117,298] color=0xff000000\n"
      "FillRect [116,0,123,298] color=0xff000000\n"
      "FillRect [123,0,130,298] color=0xff000000\n"
      "FillRect [135,0,142,298] color=0xff000000\n"
      "FillRect [141,0,148,298] color=0xff000000\n"
      "FillRect [147,0,154,298] color=0xff000000\n"
      "FillRect [153,0,160,298] color=0xff000000\n"
      "FillRect [166,0,173,298] color=0xff000000\n"
      "FillRect [178,0,185,298] color=0xff000000\n"
      "FillRect [202,0,209,298] color=0xff000000\n"
      "FillRect [209,0,216,298] color=0xff000000\n"
      "FillRect [221,0,228,298] color=0xff000000\n"
      "FillRect [227,0,234,298] color=0xff000000\n"
      "FillRect [245,0,252,298] color=0xff000000\n"
      "FillRect [252,0,259,298] color=0xff000000\n"
      "FillRect [270,0,277,298] color=0xff000000\n"
      "FillRect [276,0,283,298] color=0xff000000\n"
      "FillRect [288,0,295,298] color=0xff000000\n"
      "FillRect [295,0,302,298] color=0xff000000\n"
      "FillRect [307,0,314,298] color=0xff000000\n"
      "FillRect [313,0,320,298] color=0xff000000\n"
      "FillRect [319,0,326,298] color=0xff000000\n"
      "FillRect [325,0,332,298] color=0xff000000\n"
      "FillRect [338,0,345,298] color=0xff000000\n"
      "FillRect [344,0,351,298] color=0xff000000\n"
      "FillRect [368,0,375,298] color=0xff000000\n"
      "FillRect [375,0,382,298] color=0xff000000\n"
      "FillRect [381,0,388,298] color=0xff000000\n"
      "FillRect [393,0,400,298] color=0xff000000\n"
      "FillRect [405,0,412,298] color=0xff000000\n"
      "FillRect [411,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, Ean8) {
  Create(BC_TYPE::kEAN8);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [0,0,7,298] color=0xff000000\n"
      "FillRect [12,0,19,298] color=0xff000000\n"
      "FillRect [37,0,44,298] color=0xff000000\n"
      "FillRect [43,0,50,298] color=0xff000000\n"
      "FillRect [56,0,63,298] color=0xff000000\n"
      "FillRect [74,0,81,298] color=0xff000000\n"
      "FillRect [81,0,88,298] color=0xff000000\n"
      "FillRect [99,0,106,298] color=0xff000000\n"
      "FillRect [118,0,125,298] color=0xff000000\n"
      "FillRect [137,0,144,298] color=0xff000000\n"
      "FillRect [143,0,150,298] color=0xff000000\n"
      "FillRect [156,0,163,298] color=0xff000000\n"
      "FillRect [162,0,169,298] color=0xff000000\n"
      "FillRect [168,0,175,298] color=0xff000000\n"
      "FillRect [174,0,181,298] color=0xff000000\n"
      "FillRect [187,0,194,298] color=0xff000000\n"
      "FillRect [199,0,206,298] color=0xff000000\n"
      "FillRect [212,0,219,298] color=0xff000000\n"
      "FillRect [224,0,231,298] color=0xff000000\n"
      "FillRect [237,0,244,298] color=0xff000000\n"
      "FillRect [243,0,250,298] color=0xff000000\n"
      "FillRect [249,0,256,298] color=0xff000000\n"
      "FillRect [268,0,275,298] color=0xff000000\n"
      "FillRect [287,0,294,298] color=0xff000000\n"
      "FillRect [293,0,300,298] color=0xff000000\n"
      "FillRect [299,0,306,298] color=0xff000000\n"
      "FillRect [312,0,319,298] color=0xff000000\n"
      "FillRect [324,0,331,298] color=0xff000000\n"
      "FillRect [355,0,362,298] color=0xff000000\n"
      "FillRect [374,0,381,298] color=0xff000000\n"
      "FillRect [380,0,387,298] color=0xff000000\n"
      "FillRect [386,0,393,298] color=0xff000000\n"
      "FillRect [399,0,406,298] color=0xff000000\n"
      "FillRect [411,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, Ean8Letters) {
  Create(BC_TYPE::kEAN8);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, UPCA) {
  Create(BC_TYPE::kUPCA);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [27,0,31,298] color=0xff000000\n"
      "FillRect [34,0,38,298] color=0xff000000\n"
      "FillRect [50,0,54,298] color=0xff000000\n"
      "FillRect [54,0,58,298] color=0xff000000\n"
      "FillRect [61,0,65,298] color=0xff000000\n"
      "FillRect [77,0,81,298] color=0xff000000\n"
      "FillRect [80,0,84,298] color=0xff000000\n"
      "FillRect [88,0,92,298] color=0xff000000\n"
      "FillRect [103,0,107,298] color=0xff000000\n"
      "FillRect [107,0,111,298] color=0xff000000\n"
      "FillRect [115,0,119,298] color=0xff000000\n"
      "FillRect [130,0,134,298] color=0xff000000\n"
      "FillRect [134,0,138,298] color=0xff000000\n"
      "FillRect [142,0,146,298] color=0xff000000\n"
      "FillRect [157,0,161,298] color=0xff000000\n"
      "FillRect [161,0,165,298] color=0xff000000\n"
      "FillRect [169,0,173,298] color=0xff000000\n"
      "FillRect [180,0,184,298] color=0xff000000\n"
      "FillRect [184,0,188,298] color=0xff000000\n"
      "FillRect [195,0,199,298] color=0xff000000\n"
      "FillRect [203,0,207,298] color=0xff000000\n"
      "FillRect [211,0,215,298] color=0xff000000\n"
      "FillRect [219,0,223,298] color=0xff000000\n"
      "FillRect [222,0,226,298] color=0xff000000\n"
      "FillRect [230,0,234,298] color=0xff000000\n"
      "FillRect [234,0,238,298] color=0xff000000\n"
      "FillRect [245,0,249,298] color=0xff000000\n"
      "FillRect [265,0,269,298] color=0xff000000\n"
      "FillRect [272,0,276,298] color=0xff000000\n"
      "FillRect [280,0,284,298] color=0xff000000\n"
      "FillRect [284,0,288,298] color=0xff000000\n"
      "FillRect [288,0,292,298] color=0xff000000\n"
      "FillRect [299,0,303,298] color=0xff000000\n"
      "FillRect [311,0,315,298] color=0xff000000\n"
      "FillRect [314,0,318,298] color=0xff000000\n"
      "FillRect [318,0,322,298] color=0xff000000\n"
      "FillRect [326,0,330,298] color=0xff000000\n"
      "FillRect [334,0,338,298] color=0xff000000\n"
      "FillRect [353,0,357,298] color=0xff000000\n"
      "FillRect [364,0,368,298] color=0xff000000\n"
      "FillRect [368,0,372,298] color=0xff000000\n"
      "FillRect [372,0,376,298] color=0xff000000\n"
      "FillRect [380,0,384,298] color=0xff000000\n"
      "FillRect [387,0,391,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, UPCALetters) {
  Create(BC_TYPE::kUPCA);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, Ean13) {
  Create(BC_TYPE::kEAN13);
  EXPECT_TRUE(barcode()->Encode(L"123456"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [28,0,33,298] color=0xff000000\n"
      "FillRect [36,0,41,298] color=0xff000000\n"
      "FillRect [53,0,58,298] color=0xff000000\n"
      "FillRect [57,0,62,298] color=0xff000000\n"
      "FillRect [65,0,70,298] color=0xff000000\n"
      "FillRect [82,0,87,298] color=0xff000000\n"
      "FillRect [86,0,91,298] color=0xff000000\n"
      "FillRect [94,0,99,298] color=0xff000000\n"
      "FillRect [110,0,115,298] color=0xff000000\n"
      "FillRect [114,0,119,298] color=0xff000000\n"
      "FillRect [122,0,127,298] color=0xff000000\n"
      "FillRect [139,0,144,298] color=0xff000000\n"
      "FillRect [143,0,148,298] color=0xff000000\n"
      "FillRect [151,0,156,298] color=0xff000000\n"
      "FillRect [168,0,173,298] color=0xff000000\n"
      "FillRect [172,0,177,298] color=0xff000000\n"
      "FillRect [180,0,185,298] color=0xff000000\n"
      "FillRect [192,0,197,298] color=0xff000000\n"
      "FillRect [196,0,201,298] color=0xff000000\n"
      "FillRect [209,0,214,298] color=0xff000000\n"
      "FillRect [217,0,222,298] color=0xff000000\n"
      "FillRect [225,0,230,298] color=0xff000000\n"
      "FillRect [233,0,238,298] color=0xff000000\n"
      "FillRect [237,0,242,298] color=0xff000000\n"
      "FillRect [245,0,250,298] color=0xff000000\n"
      "FillRect [250,0,255,298] color=0xff000000\n"
      "FillRect [262,0,267,298] color=0xff000000\n"
      "FillRect [282,0,287,298] color=0xff000000\n"
      "FillRect [291,0,296,298] color=0xff000000\n"
      "FillRect [299,0,304,298] color=0xff000000\n"
      "FillRect [303,0,308,298] color=0xff000000\n"
      "FillRect [307,0,312,298] color=0xff000000\n"
      "FillRect [319,0,324,298] color=0xff000000\n"
      "FillRect [331,0,336,298] color=0xff000000\n"
      "FillRect [336,0,341,298] color=0xff000000\n"
      "FillRect [340,0,345,298] color=0xff000000\n"
      "FillRect [348,0,353,298] color=0xff000000\n"
      "FillRect [356,0,361,298] color=0xff000000\n"
      "FillRect [377,0,382,298] color=0xff000000\n"
      "FillRect [389,0,394,298] color=0xff000000\n"
      "FillRect [393,0,398,298] color=0xff000000\n"
      "FillRect [397,0,402,298] color=0xff000000\n"
      "FillRect [405,0,410,298] color=0xff000000\n"
      "FillRect [413,0,418,298] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, Ean13Letters) {
  Create(BC_TYPE::kEAN13);
  EXPECT_FALSE(barcode()->Encode(L"clams"));
}

TEST_F(BarcodeTest, Pdf417) {
  Create(BC_TYPE::kPDF417);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [54,44,57,86] color=0xff000000\n"
      "FillRect [54,86,57,128] color=0xff000000\n"
      "FillRect [54,128,57,170] color=0xff000000\n"
      "FillRect [54,170,57,212] color=0xff000000\n"
      "FillRect [54,212,57,254] color=0xff000000\n"
      "FillRect [57,44,60,86] color=0xff000000\n"
      "FillRect [57,86,60,128] color=0xff000000\n"
      "FillRect [57,128,60,170] color=0xff000000\n"
      "FillRect [57,170,60,212] color=0xff000000\n"
      "FillRect [57,212,60,254] color=0xff000000\n"
      "FillRect [60,44,63,86] color=0xff000000\n"
      "FillRect [60,86,63,128] color=0xff000000\n"
      "FillRect [60,128,63,170] color=0xff000000\n"
      "FillRect [60,170,63,212] color=0xff000000\n"
      "FillRect [60,212,63,254] color=0xff000000\n"
      "FillRect [63,44,66,86] color=0xff000000\n"
      "FillRect [63,86,66,128] color=0xff000000\n"
      "FillRect [63,128,66,170] color=0xff000000\n"
      "FillRect [63,170,66,212] color=0xff000000\n"
      "FillRect [63,212,66,254] color=0xff000000\n"
      "FillRect [66,44,69,86] color=0xff000000\n"
      "FillRect [66,86,69,128] color=0xff000000\n"
      "FillRect [66,128,69,170] color=0xff000000\n"
      "FillRect [66,170,69,212] color=0xff000000\n"
      "FillRect [66,212,69,254] color=0xff000000\n"
      "FillRect [69,44,72,86] color=0xff000000\n"
      "FillRect [69,86,72,128] color=0xff000000\n"
      "FillRect [69,128,72,170] color=0xff000000\n"
      "FillRect [69,170,72,212] color=0xff000000\n"
      "FillRect [69,212,72,254] color=0xff000000\n"
      "FillRect [72,44,75,86] color=0xff000000\n"
      "FillRect [72,86,75,128] color=0xff000000\n"
      "FillRect [72,128,75,170] color=0xff000000\n"
      "FillRect [72,170,75,212] color=0xff000000\n"
      "FillRect [72,212,75,254] color=0xff000000\n"
      "FillRect [75,44,78,86] color=0xff000000\n"
      "FillRect [75,86,78,128] color=0xff000000\n"
      "FillRect [75,128,78,170] color=0xff000000\n"
      "FillRect [75,170,78,212] color=0xff000000\n"
      "FillRect [75,212,78,254] color=0xff000000\n"
      "FillRect [81,44,84,86] color=0xff000000\n"
      "FillRect [81,86,84,128] color=0xff000000\n"
      "FillRect [81,128,84,170] color=0xff000000\n"
      "FillRect [81,170,84,212] color=0xff000000\n"
      "FillRect [81,212,84,254] color=0xff000000\n"
      "FillRect [87,44,90,86] color=0xff000000\n"
      "FillRect [87,86,90,128] color=0xff000000\n"
      "FillRect [87,128,90,170] color=0xff000000\n"
      "FillRect [87,170,90,212] color=0xff000000\n"
      "FillRect [87,212,90,254] color=0xff000000\n"
      "FillRect [93,44,96,86] color=0xff000000\n"
      "FillRect [93,86,96,128] color=0xff000000\n"
      "FillRect [93,128,96,170] color=0xff000000\n"
      "FillRect [93,170,96,212] color=0xff000000\n"
      "FillRect [93,212,96,254] color=0xff000000\n"
      "FillRect [105,44,108,86] color=0xff000000\n"
      "FillRect [105,86,108,128] color=0xff000000\n"
      "FillRect [105,128,108,170] color=0xff000000\n"
      "FillRect [105,170,108,212] color=0xff000000\n"
      "FillRect [105,212,108,254] color=0xff000000\n"
      "FillRect [108,44,111,86] color=0xff000000\n"
      "FillRect [108,86,111,128] color=0xff000000\n"
      "FillRect [108,128,111,170] color=0xff000000\n"
      "FillRect [108,212,111,254] color=0xff000000\n"
      "FillRect [111,44,114,86] color=0xff000000\n"
      "FillRect [111,86,114,128] color=0xff000000\n"
      "FillRect [111,128,114,170] color=0xff000000\n"
      "FillRect [111,170,114,212] color=0xff000000\n"
      "FillRect [111,212,114,254] color=0xff000000\n"
      "FillRect [114,44,117,86] color=0xff000000\n"
      "FillRect [114,86,117,128] color=0xff000000\n"
      "FillRect [117,86,120,128] color=0xff000000\n"
      "FillRect [117,128,120,170] color=0xff000000\n"
      "FillRect [117,170,120,212] color=0xff000000\n"
      "FillRect [117,212,120,254] color=0xff000000\n"
      "FillRect [120,44,123,86] color=0xff000000\n"
      "FillRect [120,86,123,128] color=0xff000000\n"
      "FillRect [120,170,123,212] color=0xff000000\n"
      "FillRect [123,128,126,170] color=0xff000000\n"
      "FillRect [123,170,126,212] color=0xff000000\n"
      "FillRect [123,212,126,254] color=0xff000000\n"
      "FillRect [126,44,129,86] color=0xff000000\n"
      "FillRect [126,86,129,128] color=0xff000000\n"
      "FillRect [126,170,129,212] color=0xff000000\n"
      "FillRect [126,212,129,254] color=0xff000000\n"
      "FillRect [129,128,132,170] color=0xff000000\n"
      "FillRect [129,212,132,254] color=0xff000000\n"
      "FillRect [132,44,135,86] color=0xff000000\n"
      "FillRect [132,86,135,128] color=0xff000000\n"
      "FillRect [132,128,135,170] color=0xff000000\n"
      "FillRect [135,44,138,86] color=0xff000000\n"
      "FillRect [135,128,138,170] color=0xff000000\n"
      "FillRect [135,170,138,212] color=0xff000000\n"
      "FillRect [138,44,141,86] color=0xff000000\n"
      "FillRect [138,128,141,170] color=0xff000000\n"
      "FillRect [138,170,141,212] color=0xff000000\n"
      "FillRect [141,44,144,86] color=0xff000000\n"
      "FillRect [141,86,144,128] color=0xff000000\n"
      "FillRect [141,128,144,170] color=0xff000000\n"
      "FillRect [141,170,144,212] color=0xff000000\n"
      "FillRect [141,212,144,254] color=0xff000000\n"
      "FillRect [144,86,147,128] color=0xff000000\n"
      "FillRect [144,128,147,170] color=0xff000000\n"
      "FillRect [144,170,147,212] color=0xff000000\n"
      "FillRect [144,212,147,254] color=0xff000000\n"
      "FillRect [147,86,150,128] color=0xff000000\n"
      "FillRect [156,44,159,86] color=0xff000000\n"
      "FillRect [156,86,159,128] color=0xff000000\n"
      "FillRect [156,128,159,170] color=0xff000000\n"
      "FillRect [156,170,159,212] color=0xff000000\n"
      "FillRect [156,212,159,254] color=0xff000000\n"
      "FillRect [159,44,162,86] color=0xff000000\n"
      "FillRect [159,170,162,212] color=0xff000000\n"
      "FillRect [165,44,168,86] color=0xff000000\n"
      "FillRect [165,128,168,170] color=0xff000000\n"
      "FillRect [165,170,168,212] color=0xff000000\n"
      "FillRect [168,128,171,170] color=0xff000000\n"
      "FillRect [168,170,171,212] color=0xff000000\n"
      "FillRect [168,212,171,254] color=0xff000000\n"
      "FillRect [171,44,174,86] color=0xff000000\n"
      "FillRect [174,86,177,128] color=0xff000000\n"
      "FillRect [174,170,177,212] color=0xff000000\n"
      "FillRect [174,212,177,254] color=0xff000000\n"
      "FillRect [177,128,180,170] color=0xff000000\n"
      "FillRect [177,212,180,254] color=0xff000000\n"
      "FillRect [180,86,183,128] color=0xff000000\n"
      "FillRect [180,212,183,254] color=0xff000000\n"
      "FillRect [183,44,186,86] color=0xff000000\n"
      "FillRect [183,86,186,128] color=0xff000000\n"
      "FillRect [183,212,186,254] color=0xff000000\n"
      "FillRect [186,44,189,86] color=0xff000000\n"
      "FillRect [186,86,189,128] color=0xff000000\n"
      "FillRect [186,212,189,254] color=0xff000000\n"
      "FillRect [189,86,192,128] color=0xff000000\n"
      "FillRect [192,170,195,212] color=0xff000000\n"
      "FillRect [195,86,198,128] color=0xff000000\n"
      "FillRect [195,128,198,170] color=0xff000000\n"
      "FillRect [195,212,198,254] color=0xff000000\n"
      "FillRect [198,128,201,170] color=0xff000000\n"
      "FillRect [198,212,201,254] color=0xff000000\n"
      "FillRect [201,128,204,170] color=0xff000000\n"
      "FillRect [207,44,210,86] color=0xff000000\n"
      "FillRect [207,86,210,128] color=0xff000000\n"
      "FillRect [207,128,210,170] color=0xff000000\n"
      "FillRect [207,170,210,212] color=0xff000000\n"
      "FillRect [207,212,210,254] color=0xff000000\n"
      "FillRect [210,128,213,170] color=0xff000000\n"
      "FillRect [210,170,213,212] color=0xff000000\n"
      "FillRect [210,212,213,254] color=0xff000000\n"
      "FillRect [213,170,216,212] color=0xff000000\n"
      "FillRect [213,212,216,254] color=0xff000000\n"
      "FillRect [216,86,219,128] color=0xff000000\n"
      "FillRect [216,170,219,212] color=0xff000000\n"
      "FillRect [216,212,219,254] color=0xff000000\n"
      "FillRect [222,44,225,86] color=0xff000000\n"
      "FillRect [222,128,225,170] color=0xff000000\n"
      "FillRect [225,44,228,86] color=0xff000000\n"
      "FillRect [225,86,228,128] color=0xff000000\n"
      "FillRect [225,128,228,170] color=0xff000000\n"
      "FillRect [225,170,228,212] color=0xff000000\n"
      "FillRect [225,212,228,254] color=0xff000000\n"
      "FillRect [228,86,231,128] color=0xff000000\n"
      "FillRect [228,128,231,170] color=0xff000000\n"
      "FillRect [228,170,231,212] color=0xff000000\n"
      "FillRect [231,86,234,128] color=0xff000000\n"
      "FillRect [231,128,234,170] color=0xff000000\n"
      "FillRect [231,170,234,212] color=0xff000000\n"
      "FillRect [234,86,237,128] color=0xff000000\n"
      "FillRect [234,128,237,170] color=0xff000000\n"
      "FillRect [237,44,240,86] color=0xff000000\n"
      "FillRect [237,170,240,212] color=0xff000000\n"
      "FillRect [237,212,240,254] color=0xff000000\n"
      "FillRect [240,44,243,86] color=0xff000000\n"
      "FillRect [243,128,246,170] color=0xff000000\n"
      "FillRect [246,170,249,212] color=0xff000000\n"
      "FillRect [249,44,252,86] color=0xff000000\n"
      "FillRect [249,170,252,212] color=0xff000000\n"
      "FillRect [252,86,255,128] color=0xff000000\n"
      "FillRect [252,128,255,170] color=0xff000000\n"
      "FillRect [252,212,255,254] color=0xff000000\n"
      "FillRect [258,44,261,86] color=0xff000000\n"
      "FillRect [258,86,261,128] color=0xff000000\n"
      "FillRect [258,128,261,170] color=0xff000000\n"
      "FillRect [258,170,261,212] color=0xff000000\n"
      "FillRect [258,212,261,254] color=0xff000000\n"
      "FillRect [261,44,264,86] color=0xff000000\n"
      "FillRect [261,86,264,128] color=0xff000000\n"
      "FillRect [261,128,264,170] color=0xff000000\n"
      "FillRect [261,212,264,254] color=0xff000000\n"
      "FillRect [264,44,267,86] color=0xff000000\n"
      "FillRect [264,86,267,128] color=0xff000000\n"
      "FillRect [264,128,267,170] color=0xff000000\n"
      "FillRect [264,170,267,212] color=0xff000000\n"
      "FillRect [264,212,267,254] color=0xff000000\n"
      "FillRect [267,44,270,86] color=0xff000000\n"
      "FillRect [267,86,270,128] color=0xff000000\n"
      "FillRect [270,86,273,128] color=0xff000000\n"
      "FillRect [270,128,273,170] color=0xff000000\n"
      "FillRect [270,170,273,212] color=0xff000000\n"
      "FillRect [270,212,273,254] color=0xff000000\n"
      "FillRect [273,44,276,86] color=0xff000000\n"
      "FillRect [273,86,276,128] color=0xff000000\n"
      "FillRect [273,170,276,212] color=0xff000000\n"
      "FillRect [276,128,279,170] color=0xff000000\n"
      "FillRect [276,170,279,212] color=0xff000000\n"
      "FillRect [276,212,279,254] color=0xff000000\n"
      "FillRect [279,44,282,86] color=0xff000000\n"
      "FillRect [279,86,282,128] color=0xff000000\n"
      "FillRect [279,170,282,212] color=0xff000000\n"
      "FillRect [279,212,282,254] color=0xff000000\n"
      "FillRect [282,212,285,254] color=0xff000000\n"
      "FillRect [285,44,288,86] color=0xff000000\n"
      "FillRect [285,86,288,128] color=0xff000000\n"
      "FillRect [285,128,288,170] color=0xff000000\n"
      "FillRect [288,44,291,86] color=0xff000000\n"
      "FillRect [288,128,291,170] color=0xff000000\n"
      "FillRect [288,170,291,212] color=0xff000000\n"
      "FillRect [291,44,294,86] color=0xff000000\n"
      "FillRect [291,86,294,128] color=0xff000000\n"
      "FillRect [291,128,294,170] color=0xff000000\n"
      "FillRect [291,170,294,212] color=0xff000000\n"
      "FillRect [291,212,294,254] color=0xff000000\n"
      "FillRect [294,44,297,86] color=0xff000000\n"
      "FillRect [294,86,297,128] color=0xff000000\n"
      "FillRect [294,128,297,170] color=0xff000000\n"
      "FillRect [294,170,297,212] color=0xff000000\n"
      "FillRect [294,212,297,254] color=0xff000000\n"
      "FillRect [297,86,300,128] color=0xff000000\n"
      "FillRect [297,128,300,170] color=0xff000000\n"
      "FillRect [297,170,300,212] color=0xff000000\n"
      "FillRect [300,128,303,170] color=0xff000000\n"
      "FillRect [309,44,312,86] color=0xff000000\n"
      "FillRect [309,86,312,128] color=0xff000000\n"
      "FillRect [309,128,312,170] color=0xff000000\n"
      "FillRect [309,170,312,212] color=0xff000000\n"
      "FillRect [309,212,312,254] color=0xff000000\n"
      "FillRect [312,44,315,86] color=0xff000000\n"
      "FillRect [312,86,315,128] color=0xff000000\n"
      "FillRect [312,128,315,170] color=0xff000000\n"
      "FillRect [312,170,315,212] color=0xff000000\n"
      "FillRect [312,212,315,254] color=0xff000000\n"
      "FillRect [315,44,318,86] color=0xff000000\n"
      "FillRect [315,86,318,128] color=0xff000000\n"
      "FillRect [315,128,318,170] color=0xff000000\n"
      "FillRect [315,170,318,212] color=0xff000000\n"
      "FillRect [315,212,318,254] color=0xff000000\n"
      "FillRect [318,44,321,86] color=0xff000000\n"
      "FillRect [318,86,321,128] color=0xff000000\n"
      "FillRect [318,128,321,170] color=0xff000000\n"
      "FillRect [318,170,321,212] color=0xff000000\n"
      "FillRect [318,212,321,254] color=0xff000000\n"
      "FillRect [321,44,324,86] color=0xff000000\n"
      "FillRect [321,86,324,128] color=0xff000000\n"
      "FillRect [321,128,324,170] color=0xff000000\n"
      "FillRect [321,170,324,212] color=0xff000000\n"
      "FillRect [321,212,324,254] color=0xff000000\n"
      "FillRect [324,44,327,86] color=0xff000000\n"
      "FillRect [324,86,327,128] color=0xff000000\n"
      "FillRect [324,128,327,170] color=0xff000000\n"
      "FillRect [324,170,327,212] color=0xff000000\n"
      "FillRect [324,212,327,254] color=0xff000000\n"
      "FillRect [327,44,330,86] color=0xff000000\n"
      "FillRect [327,86,330,128] color=0xff000000\n"
      "FillRect [327,128,330,170] color=0xff000000\n"
      "FillRect [327,170,330,212] color=0xff000000\n"
      "FillRect [327,212,330,254] color=0xff000000\n"
      "FillRect [333,44,336,86] color=0xff000000\n"
      "FillRect [333,86,336,128] color=0xff000000\n"
      "FillRect [333,128,336,170] color=0xff000000\n"
      "FillRect [333,170,336,212] color=0xff000000\n"
      "FillRect [333,212,336,254] color=0xff000000\n"
      "FillRect [345,44,348,86] color=0xff000000\n"
      "FillRect [345,86,348,128] color=0xff000000\n"
      "FillRect [345,128,348,170] color=0xff000000\n"
      "FillRect [345,170,348,212] color=0xff000000\n"
      "FillRect [345,212,348,254] color=0xff000000\n"
      "FillRect [351,44,354,86] color=0xff000000\n"
      "FillRect [351,86,354,128] color=0xff000000\n"
      "FillRect [351,128,354,170] color=0xff000000\n"
      "FillRect [351,170,354,212] color=0xff000000\n"
      "FillRect [351,212,354,254] color=0xff000000\n"
      "FillRect [360,44,363,86] color=0xff000000\n"
      "FillRect [360,86,363,128] color=0xff000000\n"
      "FillRect [360,128,363,170] color=0xff000000\n"
      "FillRect [360,170,363,212] color=0xff000000\n"
      "FillRect [360,212,363,254] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, DataMatrix) {
  Create(BC_TYPE::kDataMatrix);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [203,143,204,144] color=0xff000000\n"
      "FillRect [203,144,204,145] color=0xff000000\n"
      "FillRect [203,145,204,146] color=0xff000000\n"
      "FillRect [203,146,204,147] color=0xff000000\n"
      "FillRect [203,147,204,148] color=0xff000000\n"
      "FillRect [203,148,204,149] color=0xff000000\n"
      "FillRect [203,149,204,150] color=0xff000000\n"
      "FillRect [203,150,204,151] color=0xff000000\n"
      "FillRect [203,151,204,152] color=0xff000000\n"
      "FillRect [203,152,204,153] color=0xff000000\n"
      "FillRect [203,153,204,154] color=0xff000000\n"
      "FillRect [203,154,204,155] color=0xff000000\n"
      "FillRect [204,145,205,146] color=0xff000000\n"
      "FillRect [204,146,205,147] color=0xff000000\n"
      "FillRect [204,151,205,152] color=0xff000000\n"
      "FillRect [204,153,205,154] color=0xff000000\n"
      "FillRect [204,154,205,155] color=0xff000000\n"
      "FillRect [205,143,206,144] color=0xff000000\n"
      "FillRect [205,144,206,145] color=0xff000000\n"
      "FillRect [205,150,206,151] color=0xff000000\n"
      "FillRect [205,152,206,153] color=0xff000000\n"
      "FillRect [205,154,206,155] color=0xff000000\n"
      "FillRect [206,145,207,146] color=0xff000000\n"
      "FillRect [206,146,207,147] color=0xff000000\n"
      "FillRect [206,148,207,149] color=0xff000000\n"
      "FillRect [206,149,207,150] color=0xff000000\n"
      "FillRect [206,150,207,151] color=0xff000000\n"
      "FillRect [206,151,207,152] color=0xff000000\n"
      "FillRect [206,154,207,155] color=0xff000000\n"
      "FillRect [207,143,208,144] color=0xff000000\n"
      "FillRect [207,144,208,145] color=0xff000000\n"
      "FillRect [207,146,208,147] color=0xff000000\n"
      "FillRect [207,147,208,148] color=0xff000000\n"
      "FillRect [207,148,208,149] color=0xff000000\n"
      "FillRect [207,151,208,152] color=0xff000000\n"
      "FillRect [207,154,208,155] color=0xff000000\n"
      "FillRect [208,145,209,146] color=0xff000000\n"
      "FillRect [208,146,209,147] color=0xff000000\n"
      "FillRect [208,148,209,149] color=0xff000000\n"
      "FillRect [208,150,209,151] color=0xff000000\n"
      "FillRect [208,152,209,153] color=0xff000000\n"
      "FillRect [208,153,209,154] color=0xff000000\n"
      "FillRect [208,154,209,155] color=0xff000000\n"
      "FillRect [209,143,210,144] color=0xff000000\n"
      "FillRect [209,144,210,145] color=0xff000000\n"
      "FillRect [209,145,210,146] color=0xff000000\n"
      "FillRect [209,148,210,149] color=0xff000000\n"
      "FillRect [209,150,210,151] color=0xff000000\n"
      "FillRect [209,151,210,152] color=0xff000000\n"
      "FillRect [209,152,210,153] color=0xff000000\n"
      "FillRect [209,154,210,155] color=0xff000000\n"
      "FillRect [210,145,211,146] color=0xff000000\n"
      "FillRect [210,147,211,148] color=0xff000000\n"
      "FillRect [210,150,211,151] color=0xff000000\n"
      "FillRect [210,153,211,154] color=0xff000000\n"
      "FillRect [210,154,211,155] color=0xff000000\n"
      "FillRect [211,143,212,144] color=0xff000000\n"
      "FillRect [211,144,212,145] color=0xff000000\n"
      "FillRect [211,147,212,148] color=0xff000000\n"
      "FillRect [211,150,212,151] color=0xff000000\n"
      "FillRect [211,151,212,152] color=0xff000000\n"
      "FillRect [211,152,212,153] color=0xff000000\n"
      "FillRect [211,154,212,155] color=0xff000000\n"
      "FillRect [212,145,213,146] color=0xff000000\n"
      "FillRect [212,146,213,147] color=0xff000000\n"
      "FillRect [212,149,213,150] color=0xff000000\n"
      "FillRect [212,151,213,152] color=0xff000000\n"
      "FillRect [212,152,213,153] color=0xff000000\n"
      "FillRect [212,154,213,155] color=0xff000000\n"
      "FillRect [213,143,214,144] color=0xff000000\n"
      "FillRect [213,144,214,145] color=0xff000000\n"
      "FillRect [213,151,214,152] color=0xff000000\n"
      "FillRect [213,153,214,154] color=0xff000000\n"
      "FillRect [213,154,214,155] color=0xff000000\n"
      "FillRect [214,144,215,145] color=0xff000000\n"
      "FillRect [214,146,215,147] color=0xff000000\n"
      "FillRect [214,148,215,149] color=0xff000000\n"
      "FillRect [214,150,215,151] color=0xff000000\n"
      "FillRect [214,152,215,153] color=0xff000000\n"
      "FillRect [214,154,215,155] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, QrCode) {
  Create(BC_TYPE::kQRCode);
  EXPECT_TRUE(barcode()->Encode(L"clams"));
  RenderDevice();
  static constexpr char kExpected[] =
      "DrawPath fill=0xffffffff stroke=0xffffffff rule=even-odd "
      "stroke_state=[width=1.0000 cap=butt join=miter miter=10.0000] "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "points=[move(0.0000,0.0000) line(0.0000,298.0000) "
      "line(418.0000,298.0000) line(418.0000,0.0000) "
      "line(0.0000,0.0000,close)]\n"
      "FillRect [198,138,199,139] color=0xff000000\n"
      "FillRect [198,139,199,140] color=0xff000000\n"
      "FillRect [198,140,199,141] color=0xff000000\n"
      "FillRect [198,141,199,142] color=0xff000000\n"
      "FillRect [198,142,199,143] color=0xff000000\n"
      "FillRect [198,143,199,144] color=0xff000000\n"
      "FillRect [198,144,199,145] color=0xff000000\n"
      "FillRect [198,146,199,147] color=0xff000000\n"
      "FillRect [198,147,199,148] color=0xff000000\n"
      "FillRect [198,149,199,150] color=0xff000000\n"
      "FillRect [198,152,199,153] color=0xff000000\n"
      "FillRect [198,153,199,154] color=0xff000000\n"
      "FillRect [198,154,199,155] color=0xff000000\n"
      "FillRect [198,155,199,156] color=0xff000000\n"
      "FillRect [198,156,199,157] color=0xff000000\n"
      "FillRect [198,157,199,158] color=0xff000000\n"
      "FillRect [198,158,199,159] color=0xff000000\n"
      "FillRect [199,138,200,139] color=0xff000000\n"
      "FillRect [199,144,200,145] color=0xff000000\n"
      "FillRect [199,147,200,148] color=0xff000000\n"
      "FillRect [199,152,200,153] color=0xff000000\n"
      "FillRect [199,158,200,159] color=0xff000000\n"
      "FillRect [200,138,201,139] color=0xff000000\n"
      "FillRect [200,140,201,141] color=0xff000000\n"
      "FillRect [200,141,201,142] color=0xff000000\n"
      "FillRect [200,142,201,143] color=0xff000000\n"
      "FillRect [200,144,201,145] color=0xff000000\n"
      "FillRect [200,146,201,147] color=0xff000000\n"
      "FillRect [200,148,201,149] color=0xff000000\n"
      "FillRect [200,150,201,151] color=0xff000000\n"
      "FillRect [200,152,201,153] color=0xff000000\n"
      "FillRect [200,154,201,155] color=0xff000000\n"
      "FillRect [200,155,201,156] color=0xff000000\n"
      "FillRect [200,156,201,157] color=0xff000000\n"
      "FillRect [200,158,201,159] color=0xff000000\n"
      "FillRect [201,138,202,139] color=0xff000000\n"
      "FillRect [201,140,202,141] color=0xff000000\n"
      "FillRect [201,141,202,142] color=0xff000000\n"
      "FillRect [201,142,202,143] color=0xff000000\n"
      "FillRect [201,144,202,145] color=0xff000000\n"
      "FillRect [201,146,202,147] color=0xff000000\n"
      "FillRect [201,152,202,153] color=0xff000000\n"
      "FillRect [201,154,202,155] color=0xff000000\n"
      "FillRect [201,155,202,156] color=0xff000000\n"
      "FillRect [201,156,202,157] color=0xff000000\n"
      "FillRect [201,158,202,159] color=0xff000000\n"
      "FillRect [202,138,203,139] color=0xff000000\n"
      "FillRect [202,140,203,141] color=0xff000000\n"
      "FillRect [202,141,203,142] color=0xff000000\n"
      "FillRect [202,142,203,143] color=0xff000000\n"
      "FillRect [202,144,203,145] color=0xff000000\n"
      "FillRect [202,146,203,147] color=0xff000000\n"
      "FillRect [202,148,203,149] color=0xff000000\n"
      "FillRect [202,152,203,153] color=0xff000000\n"
      "FillRect [202,154,203,155] color=0xff000000\n"
      "FillRect [202,155,203,156] color=0xff000000\n"
      "FillRect [202,156,203,157] color=0xff000000\n"
      "FillRect [202,158,203,159] color=0xff000000\n"
      "FillRect [203,138,204,139] color=0xff000000\n"
      "FillRect [203,144,204,145] color=0xff000000\n"
      "FillRect [203,146,204,147] color=0xff000000\n"
      "FillRect [203,148,204,149] color=0xff000000\n"
      "FillRect [203,150,204,151] color=0xff000000\n"
      "FillRect [203,152,204,153] color=0xff000000\n"
      "FillRect [203,158,204,159] color=0xff000000\n"
      "FillRect [204,138,205,139] color=0xff000000\n"
      "FillRect [204,139,205,140] color=0xff000000\n"
      "FillRect [204,140,205,141] color=0xff000000\n"
      "FillRect [204,141,205,142] color=0xff000000\n"
      "FillRect [204,142,205,143] color=0xff000000\n"
      "FillRect [204,143,205,144] color=0xff000000\n"
      "FillRect [204,144,205,145] color=0xff000000\n"
      "FillRect [204,146,205,147] color=0xff000000\n"
      "FillRect [204,148,205,149] color=0xff000000\n"
      "FillRect [204,150,205,151] color=0xff000000\n"
      "FillRect [204,152,205,153] color=0xff000000\n"
      "FillRect [204,153,205,154] color=0xff000000\n"
      "FillRect [204,154,205,155] color=0xff000000\n"
      "FillRect [204,155,205,156] color=0xff000000\n"
      "FillRect [204,156,205,157] color=0xff000000\n"
      "FillRect [204,157,205,158] color=0xff000000\n"
      "FillRect [204,158,205,159] color=0xff000000\n"
      "FillRect [205,147,206,148] color=0xff000000\n"
      "FillRect [205,148,206,149] color=0xff000000\n"
      "FillRect [205,149,206,150] color=0xff000000\n"
      "FillRect [205,150,206,151] color=0xff000000\n"
      "FillRect [206,140,207,141] color=0xff000000\n"
      "FillRect [206,141,207,142] color=0xff000000\n"
      "FillRect [206,142,207,143] color=0xff000000\n"
      "FillRect [206,143,207,144] color=0xff000000\n"
      "FillRect [206,144,207,145] color=0xff000000\n"
      "FillRect [206,145,207,146] color=0xff000000\n"
      "FillRect [206,150,207,151] color=0xff000000\n"
      "FillRect [206,151,207,152] color=0xff000000\n"
      "FillRect [206,153,207,154] color=0xff000000\n"
      "FillRect [206,154,207,155] color=0xff000000\n"
      "FillRect [206,155,207,156] color=0xff000000\n"
      "FillRect [206,156,207,157] color=0xff000000\n"
      "FillRect [206,158,207,159] color=0xff000000\n"
      "FillRect [207,138,208,139] color=0xff000000\n"
      "FillRect [207,140,208,141] color=0xff000000\n"
      "FillRect [207,142,208,143] color=0xff000000\n"
      "FillRect [207,147,208,148] color=0xff000000\n"
      "FillRect [207,148,208,149] color=0xff000000\n"
      "FillRect [207,149,208,150] color=0xff000000\n"
      "FillRect [207,151,208,152] color=0xff000000\n"
      "FillRect [207,152,208,153] color=0xff000000\n"
      "FillRect [207,153,208,154] color=0xff000000\n"
      "FillRect [207,154,208,155] color=0xff000000\n"
      "FillRect [208,138,209,139] color=0xff000000\n"
      "FillRect [208,141,209,142] color=0xff000000\n"
      "FillRect [208,143,209,144] color=0xff000000\n"
      "FillRect [208,144,209,145] color=0xff000000\n"
      "FillRect [208,145,209,146] color=0xff000000\n"
      "FillRect [208,146,209,147] color=0xff000000\n"
      "FillRect [208,147,209,148] color=0xff000000\n"
      "FillRect [208,148,209,149] color=0xff000000\n"
      "FillRect [208,149,209,150] color=0xff000000\n"
      "FillRect [208,153,209,154] color=0xff000000\n"
      "FillRect [208,156,209,157] color=0xff000000\n"
      "FillRect [209,138,210,139] color=0xff000000\n"
      "FillRect [209,140,210,141] color=0xff000000\n"
      "FillRect [209,143,210,144] color=0xff000000\n"
      "FillRect [209,146,210,147] color=0xff000000\n"
      "FillRect [209,147,210,148] color=0xff000000\n"
      "FillRect [209,149,210,150] color=0xff000000\n"
      "FillRect [209,152,210,153] color=0xff000000\n"
      "FillRect [209,154,210,155] color=0xff000000\n"
      "FillRect [209,155,210,156] color=0xff000000\n"
      "FillRect [209,157,210,158] color=0xff000000\n"
      "FillRect [210,140,211,141] color=0xff000000\n"
      "FillRect [210,141,211,142] color=0xff000000\n"
      "FillRect [210,142,211,143] color=0xff000000\n"
      "FillRect [210,144,211,145] color=0xff000000\n"
      "FillRect [210,147,211,148] color=0xff000000\n"
      "FillRect [210,148,211,149] color=0xff000000\n"
      "FillRect [210,149,211,150] color=0xff000000\n"
      "FillRect [210,150,211,151] color=0xff000000\n"
      "FillRect [210,151,211,152] color=0xff000000\n"
      "FillRect [210,155,211,156] color=0xff000000\n"
      "FillRect [210,156,211,157] color=0xff000000\n"
      "FillRect [210,157,211,158] color=0xff000000\n"
      "FillRect [210,158,211,159] color=0xff000000\n"
      "FillRect [211,147,212,148] color=0xff000000\n"
      "FillRect [211,149,212,150] color=0xff000000\n"
      "FillRect [211,152,212,153] color=0xff000000\n"
      "FillRect [211,154,212,155] color=0xff000000\n"
      "FillRect [211,155,212,156] color=0xff000000\n"
      "FillRect [211,157,212,158] color=0xff000000\n"
      "FillRect [212,138,213,139] color=0xff000000\n"
      "FillRect [212,139,213,140] color=0xff000000\n"
      "FillRect [212,140,213,141] color=0xff000000\n"
      "FillRect [212,141,213,142] color=0xff000000\n"
      "FillRect [212,142,213,143] color=0xff000000\n"
      "FillRect [212,143,213,144] color=0xff000000\n"
      "FillRect [212,144,213,145] color=0xff000000\n"
      "FillRect [212,146,213,147] color=0xff000000\n"
      "FillRect [212,147,213,148] color=0xff000000\n"
      "FillRect [212,148,213,149] color=0xff000000\n"
      "FillRect [212,149,213,150] color=0xff000000\n"
      "FillRect [212,155,213,156] color=0xff000000\n"
      "FillRect [212,156,213,157] color=0xff000000\n"
      "FillRect [212,157,213,158] color=0xff000000\n"
      "FillRect [213,138,214,139] color=0xff000000\n"
      "FillRect [213,144,214,145] color=0xff000000\n"
      "FillRect [213,146,214,147] color=0xff000000\n"
      "FillRect [213,148,214,149] color=0xff000000\n"
      "FillRect [213,150,214,151] color=0xff000000\n"
      "FillRect [213,151,214,152] color=0xff000000\n"
      "FillRect [213,153,214,154] color=0xff000000\n"
      "FillRect [213,155,214,156] color=0xff000000\n"
      "FillRect [213,156,214,157] color=0xff000000\n"
      "FillRect [213,158,214,159] color=0xff000000\n"
      "FillRect [214,138,215,139] color=0xff000000\n"
      "FillRect [214,140,215,141] color=0xff000000\n"
      "FillRect [214,141,215,142] color=0xff000000\n"
      "FillRect [214,142,215,143] color=0xff000000\n"
      "FillRect [214,144,215,145] color=0xff000000\n"
      "FillRect [214,146,215,147] color=0xff000000\n"
      "FillRect [214,152,215,153] color=0xff000000\n"
      "FillRect [214,153,215,154] color=0xff000000\n"
      "FillRect [214,154,215,155] color=0xff000000\n"
      "FillRect [214,157,215,158] color=0xff000000\n"
      "FillRect [215,138,216,139] color=0xff000000\n"
      "FillRect [215,140,216,141] color=0xff000000\n"
      "FillRect [215,141,216,142] color=0xff000000\n"
      "FillRect [215,142,216,143] color=0xff000000\n"
      "FillRect [215,144,216,145] color=0xff000000\n"
      "FillRect [215,146,216,147] color=0xff000000\n"
      "FillRect [215,147,216,148] color=0xff000000\n"
      "FillRect [215,149,216,150] color=0xff000000\n"
      "FillRect [215,152,216,153] color=0xff000000\n"
      "FillRect [215,153,216,154] color=0xff000000\n"
      "FillRect [215,155,216,156] color=0xff000000\n"
      "FillRect [215,156,216,157] color=0xff000000\n"
      "FillRect [215,157,216,158] color=0xff000000\n"
      "FillRect [215,158,216,159] color=0xff000000\n"
      "FillRect [216,138,217,139] color=0xff000000\n"
      "FillRect [216,140,217,141] color=0xff000000\n"
      "FillRect [216,141,217,142] color=0xff000000\n"
      "FillRect [216,142,217,143] color=0xff000000\n"
      "FillRect [216,144,217,145] color=0xff000000\n"
      "FillRect [216,146,217,147] color=0xff000000\n"
      "FillRect [216,148,217,149] color=0xff000000\n"
      "FillRect [216,149,217,150] color=0xff000000\n"
      "FillRect [216,151,217,152] color=0xff000000\n"
      "FillRect [216,152,217,153] color=0xff000000\n"
      "FillRect [216,153,217,154] color=0xff000000\n"
      "FillRect [216,156,217,157] color=0xff000000\n"
      "FillRect [216,157,217,158] color=0xff000000\n"
      "FillRect [217,138,218,139] color=0xff000000\n"
      "FillRect [217,144,218,145] color=0xff000000\n"
      "FillRect [217,148,218,149] color=0xff000000\n"
      "FillRect [217,149,218,150] color=0xff000000\n"
      "FillRect [217,152,218,153] color=0xff000000\n"
      "FillRect [217,153,218,154] color=0xff000000\n"
      "FillRect [217,154,218,155] color=0xff000000\n"
      "FillRect [217,158,218,159] color=0xff000000\n"
      "FillRect [218,138,219,139] color=0xff000000\n"
      "FillRect [218,139,219,140] color=0xff000000\n"
      "FillRect [218,140,219,141] color=0xff000000\n"
      "FillRect [218,141,219,142] color=0xff000000\n"
      "FillRect [218,142,219,143] color=0xff000000\n"
      "FillRect [218,143,219,144] color=0xff000000\n"
      "FillRect [218,144,219,145] color=0xff000000\n"
      "FillRect [218,147,219,148] color=0xff000000\n"
      "FillRect [218,149,219,150] color=0xff000000\n"
      "FillRect [218,151,219,152] color=0xff000000\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(BarcodeTest, GetType) {
  Create(BC_TYPE::kCode39);
  EXPECT_EQ(BC_TYPE::kCode39, barcode()->GetType());

  Create(BC_TYPE::kCodabar);
  EXPECT_EQ(BC_TYPE::kCodabar, barcode()->GetType());

  Create(BC_TYPE::kCode128);
  EXPECT_EQ(BC_TYPE::kCode128, barcode()->GetType());

  Create(BC_TYPE::kCode128B);
  EXPECT_EQ(BC_TYPE::kCode128, barcode()->GetType());

  Create(BC_TYPE::kCode128C);
  EXPECT_EQ(BC_TYPE::kCode128, barcode()->GetType());

  Create(BC_TYPE::kEAN8);
  EXPECT_EQ(BC_TYPE::kEAN8, barcode()->GetType());

  Create(BC_TYPE::kUPCA);
  EXPECT_EQ(BC_TYPE::kUPCA, barcode()->GetType());

  Create(BC_TYPE::kEAN13);
  EXPECT_EQ(BC_TYPE::kEAN13, barcode()->GetType());

  Create(BC_TYPE::kQRCode);
  EXPECT_EQ(BC_TYPE::kQRCode, barcode()->GetType());

  Create(BC_TYPE::kPDF417);
  EXPECT_EQ(BC_TYPE::kPDF417, barcode()->GetType());

  Create(BC_TYPE::kDataMatrix);
  EXPECT_EQ(BC_TYPE::kDataMatrix, barcode()->GetType());
}

TEST_F(BarcodeTest, SetStartEndChar) {
  Create(BC_TYPE::kCodabar);
  EXPECT_TRUE(barcode()->SetStartChar('A'));
  EXPECT_TRUE(barcode()->SetStartChar('B'));
  EXPECT_TRUE(barcode()->SetStartChar('C'));
  EXPECT_TRUE(barcode()->SetStartChar('D'));
  EXPECT_TRUE(barcode()->SetStartChar('E'));
  EXPECT_TRUE(barcode()->SetStartChar('N'));
  EXPECT_TRUE(barcode()->SetStartChar('T'));
  EXPECT_TRUE(barcode()->SetStartChar('*'));
  EXPECT_FALSE(barcode()->SetStartChar('X'));
  EXPECT_FALSE(barcode()->SetStartChar('0'));
  EXPECT_FALSE(barcode()->SetStartChar('\0'));

  EXPECT_TRUE(barcode()->SetEndChar('A'));
  EXPECT_TRUE(barcode()->SetEndChar('B'));
  EXPECT_TRUE(barcode()->SetEndChar('C'));
  EXPECT_TRUE(barcode()->SetEndChar('D'));
  EXPECT_TRUE(barcode()->SetEndChar('E'));
  EXPECT_TRUE(barcode()->SetEndChar('N'));
  EXPECT_TRUE(barcode()->SetEndChar('T'));
  EXPECT_TRUE(barcode()->SetEndChar('*'));
  EXPECT_FALSE(barcode()->SetEndChar('X'));
  EXPECT_FALSE(barcode()->SetEndChar('0'));
  EXPECT_FALSE(barcode()->SetEndChar('\0'));

  Create(BC_TYPE::kCode39);
  EXPECT_FALSE(barcode()->SetStartChar('A'));
  EXPECT_FALSE(barcode()->SetEndChar('A'));

  Create(BC_TYPE::kQRCode);
  EXPECT_FALSE(barcode()->SetStartChar('A'));
  EXPECT_FALSE(barcode()->SetEndChar('A'));
}

TEST_F(BarcodeTest, OneDimVsTwoDimProperties) {
  Create(BC_TYPE::kCode39);
  EXPECT_TRUE(barcode()->SetPrintChecksum(true));
  EXPECT_TRUE(barcode()->SetDataLength(10));
  EXPECT_TRUE(barcode()->SetCalChecksum(true));
  EXPECT_TRUE(barcode()->SetFontSize(12.0f));
  EXPECT_TRUE(barcode()->SetFontColor(0xFF000000));
  EXPECT_TRUE(barcode()->SetWideNarrowRatio(2));
  EXPECT_TRUE(barcode()->SetWideNarrowRatio(3));
  EXPECT_FALSE(barcode()->SetWideNarrowRatio(1));
  EXPECT_FALSE(barcode()->SetWideNarrowRatio(4));

  Create(BC_TYPE::kQRCode);
  EXPECT_FALSE(barcode()->SetPrintChecksum(true));
  EXPECT_FALSE(barcode()->SetDataLength(10));
  EXPECT_FALSE(barcode()->SetCalChecksum(true));
  EXPECT_FALSE(barcode()->SetFontSize(12.0f));
  EXPECT_FALSE(barcode()->SetFontColor(0xFF000000));
  EXPECT_FALSE(barcode()->SetWideNarrowRatio(2));

  Create(BC_TYPE::kPDF417);
  EXPECT_FALSE(barcode()->SetPrintChecksum(true));
  EXPECT_FALSE(barcode()->SetDataLength(10));
  EXPECT_FALSE(barcode()->SetCalChecksum(true));
  EXPECT_FALSE(barcode()->SetFontSize(12.0f));
  EXPECT_FALSE(barcode()->SetFontColor(0xFF000000));
  EXPECT_FALSE(barcode()->SetWideNarrowRatio(2));

  Create(BC_TYPE::kDataMatrix);
  EXPECT_FALSE(barcode()->SetPrintChecksum(true));
  EXPECT_FALSE(barcode()->SetDataLength(10));
  EXPECT_FALSE(barcode()->SetCalChecksum(true));
  EXPECT_FALSE(barcode()->SetFontSize(12.0f));
  EXPECT_FALSE(barcode()->SetFontColor(0xFF000000));
  EXPECT_FALSE(barcode()->SetWideNarrowRatio(2));
}

TEST_F(BarcodeTest, ErrorCorrectionLevel) {
  Create(BC_TYPE::kQRCode);
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(0));
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(1));
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(2));
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(3));
  EXPECT_FALSE(barcode()->SetErrorCorrectionLevel(-1));
  EXPECT_FALSE(barcode()->SetErrorCorrectionLevel(4));

  Create(BC_TYPE::kPDF417);
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(0));
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(4));
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(8));
  EXPECT_FALSE(barcode()->SetErrorCorrectionLevel(-1));
  EXPECT_FALSE(barcode()->SetErrorCorrectionLevel(9));

  Create(BC_TYPE::kDataMatrix);
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(0));
  EXPECT_TRUE(barcode()->SetErrorCorrectionLevel(3));

  Create(BC_TYPE::kCode39);
  EXPECT_FALSE(barcode()->SetErrorCorrectionLevel(0));
}

TEST_F(BarcodeTest, ModuleDimensionsAndTextLocation) {
  Create(BC_TYPE::kCode39);
  EXPECT_TRUE(barcode()->SetModuleHeight(1));
  EXPECT_TRUE(barcode()->SetModuleHeight(5));
  EXPECT_TRUE(barcode()->SetModuleHeight(10));
  EXPECT_FALSE(barcode()->SetModuleHeight(0));
  EXPECT_FALSE(barcode()->SetModuleHeight(11));

  EXPECT_TRUE(barcode()->SetModuleWidth(1));
  EXPECT_TRUE(barcode()->SetModuleWidth(5));
  EXPECT_TRUE(barcode()->SetModuleWidth(10));
  EXPECT_FALSE(barcode()->SetModuleWidth(0));
  EXPECT_FALSE(barcode()->SetModuleWidth(11));

  barcode()->SetTextLocation(BC_TEXT_LOC::kNone);
  barcode()->SetTextLocation(BC_TEXT_LOC::kAbove);
  barcode()->SetTextLocation(BC_TEXT_LOC::kBelow);
  barcode()->SetTextLocation(BC_TEXT_LOC::kAboveEmbed);
  barcode()->SetTextLocation(BC_TEXT_LOC::kBelowEmbed);
}

TEST_F(BarcodeTest, EanTruncation) {
  Create(BC_TYPE::kEAN8);
  EXPECT_TRUE(barcode()->Encode(L"1234567890"));

  Create(BC_TYPE::kEAN13);
  EXPECT_TRUE(barcode()->Encode(L"1234567890123456"));

  Create(BC_TYPE::kUPCA);
  EXPECT_TRUE(barcode()->Encode(L"1234567890123456"));
}

TEST_F(BarcodeTest, Code128COddLengthPadded) {
  Create(BC_TYPE::kCode128C);
  EXPECT_TRUE(barcode()->Encode(L"123"));
  RenderDevice();
}

TEST_F(BarcodeTest, ValidationFailures) {
  Create(BC_TYPE::kCode128);
  EXPECT_FALSE(barcode()->Encode(L""));
  EXPECT_FALSE(barcode()->Encode(L"\""));

  Create(BC_TYPE::kDataMatrix);
  WideString long_str(std::wstring(3117, L'1').c_str());
  EXPECT_FALSE(barcode()->Encode(long_str.AsStringView()));
}
