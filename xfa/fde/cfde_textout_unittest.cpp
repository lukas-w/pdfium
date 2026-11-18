// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "xfa/fde/cfde_textout.h"

#include <memory>
#include <utility>

#include "build/build_config.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/display_list_device_driver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "xfa/fgas/font/cfgas_fontmgr.h"
#include "xfa/fgas/font/cfgas_gefont.h"
#include "xfa/fgas/font/cfgas_gemodule.h"

namespace pdfium {

class CFDETextOutTest : public testing::Test {
 public:
  CFDETextOutTest() = default;
  ~CFDETextOutTest() override = default;

  void SetUp() override {
    CFX_Size device_size = GetDeviceSize();
    auto driver = std::make_unique<DisplayListDeviceDriver>(device_size.width,
                                                            device_size.height);
    driver_ = driver.get();
    device_ = CFX_RenderDevice::CreateWithDriver(std::move(driver));
    ASSERT_TRUE(device_);

    font_ = LoadFont();
    ASSERT_TRUE(font_);

    text_out_ = std::make_unique<CFDE_TextOut>();
    text_out_->SetFont(font_);
    text_out_->SetFontSize(12.0f);

    EXPECT_TRUE(driver_->display_list().IsEmpty());
  }

  void TearDown() override {
    // reverse order from SetUp()
    text_out_.reset();
    font_.Reset();
    driver_ = nullptr;
    device_.reset();
  }

  virtual RetainPtr<CFGAS_GEFont> LoadFont() {
    const wchar_t kFontFamily[] = L"Arimo Bold";
    return CFGAS_GEFont::LoadFont(kFontFamily, /*dwFontStyles=*/0,
                                  FX_CodePage::kDefANSI);
  }

  virtual CFX_Size GetDeviceSize() { return CFX_Size(200, 100); }

  CFX_RenderDevice* device() { return device_.get(); }
  const ByteString& display_list() const { return driver_->display_list(); }
  CFDE_TextOut& text_out() { return *text_out_; }

 private:
  std::unique_ptr<CFX_RenderDevice> device_;
  DisplayListDeviceDriver* driver_ = nullptr;
  RetainPtr<CFGAS_GEFont> font_;
  std::unique_ptr<CFDE_TextOut> text_out_;
};

TEST_F(CFDETextOutTest, DrawLogicTextBasic) {
  text_out().DrawLogicText(device(), L"foo", CFX_RectF(0, 0, 2100, 100));
  static constexpr char kExpected[] =
      "SaveState\n"
      "DrawDeviceText font=\"Arimo\" size=-12.0000 color=0xff000000 "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "glyphs=[(idx=73,origin=(0.0000,9.7312)) "
      "(idx=82,origin=(3.9960,9.7312)) "
      "(idx=82,origin=(11.3160,9.7312))]\n"
      "RestoreState\n";
  EXPECT_EQ(kExpected, display_list());
}

TEST_F(CFDETextOutTest, DrawLogicTextEmptyRect) {
  text_out().DrawLogicText(device(), L"foo", CFX_RectF());
  EXPECT_TRUE(display_list().IsEmpty());
}

#if !BUILDFLAG(IS_WIN)
// This test depends on a particular font being present.
class CFDETextOutLargeTest : public CFDETextOutTest {
 public:
  CFDETextOutLargeTest() = default;
  ~CFDETextOutLargeTest() override = default;

  RetainPtr<CFGAS_GEFont> LoadFont() override {
    const wchar_t kFontFamily[] = L"DejaVu Sans";
    auto* font_manager = CFGAS_GEModule::Get()->GetFontMgr();
    return font_manager->LoadFont(kFontFamily, /*dwFontStyles=*/0,
                                  FX_CodePage::kFailure);
  }

  CFX_Size GetDeviceSize() override { return CFX_Size(2100, 20); }

  static constexpr char kExpectedLargeDisplayList[] =
      "SaveState\n"
      "DrawDeviceText font=\"DejaVu Sans\" size=-10.0000 color=0xff000000 "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "glyphs=[(idx=76,origin=(2079.1499,11.9794))]\n"
      "DrawDeviceText font=\"DejaVu Sans\" size=-10.0000 color=0xff000000 "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "glyphs=[(idx=5300,origin=(1954.7100,11.9794))]\n"
      "DrawDeviceText font=\"DejaVu Sans\" size=-10.0000 color=0xff000000 "
      "matrix=[1.0000,0.0000,0.0000,1.0000,0.0000,0.0000] "
      "glyphs=[(idx=54,origin=(1966.9600,11.9794)) "
      "(idx=86,origin=(1973.2999,11.9794)) "
      "(idx=86,origin=(1978.4999,11.9794)) "
      "(idx=86,origin=(1983.6998,11.9794)) "
      "(idx=86,origin=(1988.8998,11.9794)) "
      "(idx=86,origin=(1994.0997,11.9794)) "
      "(idx=86,origin=(1999.2997,11.9794)) "
      "(idx=86,origin=(2004.4996,11.9794)) "
      "(idx=86,origin=(2009.6996,11.9794)) "
      "(idx=86,origin=(2014.8995,11.9794)) "
      "(idx=86,origin=(2020.0995,11.9794)) "
      "(idx=86,origin=(2025.2994,11.9794)) "
      "(idx=86,origin=(2030.4994,11.9794)) "
      "(idx=86,origin=(2035.6993,11.9794)) "
      "(idx=86,origin=(2040.8993,11.9794))]\n"
      "RestoreState\n";
};

TEST_F(CFDETextOutLargeTest, DrawLogicTextBug953881) {
  FDE_TextStyle styles;
  styles.single_line_ = true;
  text_out().SetStyles(styles);
  text_out().SetAlignment(FDE_TextAlignment::kCenterLeft);
  text_out().SetFontSize(10.0f);

  static const wchar_t kText[] =
      L"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"
      L"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSssssssssss"
      L"sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"
      L"sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"
      L"sssssssssssssssssssssssssssssssssssssssssssssssssnnnnnnnnnnn"
      "\xfeba"
      L"Sssssssssssssssssss"
      "\xfeba"
      L"iiiiisssss";
  text_out().DrawLogicText(device(), WideString(kText),
                           CFX_RectF(3, 3, 2048, 10));
  EXPECT_EQ(kExpectedLargeDisplayList, display_list());
}

TEST_F(CFDETextOutLargeTest, DrawLogicTextBug1342078) {
  FDE_TextStyle styles;
  styles.single_line_ = true;
  text_out().SetStyles(styles);
  text_out().SetAlignment(FDE_TextAlignment::kCenterLeft);
  text_out().SetFontSize(10.0f);

  static const wchar_t kText[] =
      L"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS"
      L"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSssssssssss"
      L"sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"
      L"sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"
      L"sssssssssssssssssssssssssssssssssssssssssssssssssnnnnnnnnnnn"
      "\xfeba"
      L"Sssssssssssssssssss"
      "\xfeba"
      L"iiiiiiiiiisssss";
  text_out().DrawLogicText(device(), WideString(kText),
                           CFX_RectF(3, 3, 2048, 10));
  EXPECT_EQ(kExpectedLargeDisplayList, display_list());
}
#endif  // !BUILDFLAG(IS_WIN)

}  // namespace pdfium
