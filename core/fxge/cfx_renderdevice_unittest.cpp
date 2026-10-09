// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include <memory>
#include <utility>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_coordinates_test_support.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/cfx_fillrenderoptions.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxge/dib/fx_dib.h"
#include "core/fxge/renderdevicedriver_iface.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include <type_traits>

#include "core/fxge/cfx_gemodule.h"
#include "core/fxge/win32/cfx_psfonttracker.h"
#include "core/fxge/win32/cgdi_device_driver.h"
#endif

namespace {

class FakeDeviceDriver : public RenderDeviceDriverIface {
 public:
  FakeDeviceDriver(int width, int height) : width_(width), height_(height) {}
  ~FakeDeviceDriver() override = default;

  DeviceType GetDeviceType() const override { return DeviceType::kDisplay; }
  int GetPixelWidth() const override { return width_; }
  int GetPixelHeight() const override { return height_; }
  int GetBitsPerPixel() const override { return 32; }
  void Clear(uint32_t color) override {}
  void SaveState() override {}
  void RestoreState(bool bKeepSaved) override {}
  bool SetClip_PathFill(const CFX_Path& path,
                        const CFX_Matrix* pObject2Device,
                        const CFX_FillRenderOptions& fill_options) override {
    return true;
  }
  bool DrawPath(const CFX_Path& path,
                const CFX_Matrix* pObject2Device,
                const CFX_GraphStateData* pGraphState,
                uint32_t fill_color,
                uint32_t stroke_color,
                bool group_knockout,
                const CFX_FillRenderOptions& fill_options) override {
    return true;
  }
  FX_RECT GetClipBox() const override { return FX_RECT(0, 0, width_, height_); }
  bool SetDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                 uint32_t color,
                 const FX_RECT& src_rect,
                 int dest_left,
                 int dest_top,
                 BlendMode blend_type) override {
    return true;
  }
  bool StretchDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                     uint32_t color,
                     int dest_left,
                     int dest_top,
                     int dest_width,
                     int dest_height,
                     const FX_RECT* pClipRect,
                     const FXDIB_ResampleOptions& options,
                     BlendMode blend_type) override {
    return true;
  }
  StartResult StartDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                          float alpha,
                          uint32_t color,
                          const CFX_Matrix& matrix,
                          const FXDIB_ResampleOptions& options,
                          BlendMode blend_type) override {
    return StartResult(Result::kFailure, nullptr);
  }
  bool MultiplyAlpha(float alpha) override { return true; }
  bool MultiplyAlphaMask(RetainPtr<const CFX_DIBitmap> mask) override {
    return true;
  }

 private:
  const int width_;
  const int height_;
};

std::unique_ptr<CFX_RenderDevice> CreateDeviceForBitmap(int width, int height) {
  auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
  if (!bitmap->Create(width, height, FXDIB_Format::kBgra)) {
    return nullptr;
  }
  return CFX_RenderDevice::CreateForBitmap(std::move(bitmap));
}

}  // namespace

TEST(CFXRenderDeviceTest, CreateWithDriver) {
  EXPECT_FALSE(CFX_RenderDevice::CreateWithDriver(nullptr));

  auto driver = std::make_unique<FakeDeviceDriver>(20, 30);
  RenderDeviceDriverIface* driver_ptr = driver.get();
  std::unique_ptr<CFX_RenderDevice> device =
      CFX_RenderDevice::CreateWithDriver(std::move(driver));
  ASSERT_TRUE(device);
  EXPECT_EQ(device->GetDeviceDriver(), driver_ptr);
  EXPECT_EQ(device->GetWidth(), 20);
  EXPECT_EQ(device->GetHeight(), 30);
  EXPECT_EQ(device->GetClipBox(), FX_RECT(0, 0, 20, 30));
}

TEST(CFXRenderDeviceTest, GetClipBoxDefault) {
  std::unique_ptr<CFX_RenderDevice> device = CreateDeviceForBitmap(16, 16);
  ASSERT_TRUE(device);

  EXPECT_EQ(FX_RECT(0, 0, 16, 16), device->GetClipBox());
}

TEST(CFXRenderDeviceTest, GetClipBoxPathFill) {
  // Matrix that transposes and translates by 1 unit on each axis.
  const CFX_Matrix object_to_device(0, 1, 1, 0, 1, -1);

  // Fill type cannot be none.
  const CFX_FillRenderOptions fill_options(
      CFX_FillRenderOptions::FillType::kEvenOdd);

  std::unique_ptr<CFX_RenderDevice> device = CreateDeviceForBitmap(16, 16);
  ASSERT_TRUE(device);

  CFX_Path path;
  path.AppendRect(2, 4, 14, 12);
  EXPECT_TRUE(device->SetClip_PathFill(path, &object_to_device, fill_options));

  EXPECT_EQ(FX_RECT(5, 1, 13, 13), device->GetClipBox());
}

TEST(CFXRenderDeviceTest, GetClipBoxPathStroke) {
  // Matrix that transposes and translates by 1 unit on each axis.
  const CFX_Matrix object_to_device(0, 1, 1, 0, 1, -1);

  // Default line width is 1.
  const CFX_GraphStateData graphics_state;

  std::unique_ptr<CFX_RenderDevice> device = CreateDeviceForBitmap(16, 16);
  ASSERT_TRUE(device);

  CFX_Path path;
  path.AppendRect(2, 4, 14, 12);
  EXPECT_TRUE(
      device->SetClip_PathStroke(path, &object_to_device, &graphics_state));

  EXPECT_EQ(FX_RECT(4, 0, 14, 14), device->GetClipBox());
}

TEST(CFXRenderDeviceTest, GetClipBoxRect) {
  std::unique_ptr<CFX_RenderDevice> device = CreateDeviceForBitmap(16, 16);
  ASSERT_TRUE(device);

  EXPECT_TRUE(device->SetClip_Rect({2, 4, 14, 12}));

  EXPECT_EQ(FX_RECT(2, 4, 14, 12), device->GetClipBox());
}

TEST(CFXRenderDeviceTest, GetClipBoxEmpty) {
  std::unique_ptr<CFX_RenderDevice> device = CreateDeviceForBitmap(16, 16);
  ASSERT_TRUE(device);

  EXPECT_TRUE(device->SetClip_Rect({2, 8, 14, 8}));

  EXPECT_TRUE(device->GetClipBox().IsEmpty());
}

#if BUILDFLAG(IS_WIN)
namespace {

constexpr CFX_Matrix kIdentityMatrix;

class ScopedPrintMode {
 public:
  explicit ScopedPrintMode(WindowsPrintMode mode)
      : old_mode_(CFX_GEModule::GetPrintMode()) {
    CFX_GEModule::SetPrintMode(mode);
  }
  ~ScopedPrintMode() { CFX_GEModule::SetPrintMode(old_mode_); }

 private:
  const WindowsPrintMode old_mode_;
};

struct EnhMetaFileDCDeleter {
  void operator()(HDC dc) const { ::DeleteEnhMetaFile(::CloseEnhMetaFile(dc)); }
};
using ScopedEnhMetaFileDC =
    std::unique_ptr<std::remove_pointer_t<HDC>, EnhMetaFileDCDeleter>;

struct GdiObjectDeleter {
  void operator()(HGDIOBJ object) const { ::DeleteObject(object); }
};
template <typename T>
using ScopedGdiObject =
    std::unique_ptr<std::remove_pointer_t<T>, GdiObjectDeleter>;

}  // namespace

class CFXWindowsRenderDeviceTest : public testing::Test {
 public:
  void SetUp() override {
    testing::Test::SetUp();

    // Get a device context with Windows GDI.
    dc_handle_ = CreateCompatibleDC(nullptr);
    ASSERT_TRUE(dc_handle_);
    device_ = CFX_RenderDevice::CreateWithDriver(
        CGdiDeviceDriver::CreateDriver(dc_handle_, &psfont_tracker_));
    ASSERT_TRUE(device_);
    device_->SaveState();
  }

  void TearDown() override {
    device_->RestoreState(false);
    device_.reset();
    DeleteDC(dc_handle_);
    testing::Test::TearDown();
  }

 protected:
  HDC dc_handle_;
  CFX_PSFontTracker psfont_tracker_;
  std::unique_ptr<CFX_RenderDevice> device_;
};

TEST_F(CFXWindowsRenderDeviceTest, SimpleClipTriangle) {
  CFX_Path path_data;
  CFX_PointF p1(0.0f, 0.0f);
  CFX_PointF p2(0.0f, 100.0f);
  CFX_PointF p3(100.0f, 100.0f);

  path_data.AppendLine(p1, p2);
  path_data.AppendLine(p2, p3);
  path_data.AppendLine(p3, p1);
  path_data.ClosePath();
  EXPECT_TRUE(device_->SetClip_PathFill(
      path_data, &kIdentityMatrix, CFX_FillRenderOptions::WindingOptions()));
}

TEST_F(CFXWindowsRenderDeviceTest, SimpleClipRect) {
  CFX_Path path_data;

  path_data.AppendRect(0.0f, 100.0f, 200.0f, 0.0f);
  path_data.ClosePath();
  EXPECT_TRUE(device_->SetClip_PathFill(
      path_data, &kIdentityMatrix, CFX_FillRenderOptions::WindingOptions()));
}

TEST_F(CFXWindowsRenderDeviceTest, GargantuanClipRect) {
  CFX_Path path_data;

  path_data.AppendRect(-257698020.0f, -257697252.0f, 257698044.0f,
                       257698812.0f);
  path_data.ClosePath();
  // These coordinates for a clip path are valid, just very large. Using these
  // for a clip path should allow IntersectClipRect() to return success;
  // however they do not because the GDI API IntersectClipRect() errors out and
  // affect subsequent imaging.  crbug.com/1019026
  EXPECT_FALSE(device_->SetClip_PathFill(
      path_data, &kIdentityMatrix, CFX_FillRenderOptions::WindingOptions()));
}

TEST_F(CFXWindowsRenderDeviceTest, GargantuanClipRectWithBaseClip) {
  CFX_Path path_data;
  const FX_RECT kBaseClip(0, 0, 5100, 6600);

  device_->SetBaseClip(kBaseClip);
  path_data.AppendRect(-257698020.0f, -257697252.0f, 257698044.0f,
                       257698812.0f);
  path_data.ClosePath();
  // Use of a reasonable base clip ensures that we avoid getting an error back
  // from GDI API IntersectClipRect().
  EXPECT_TRUE(device_->SetClip_PathFill(
      path_data, &kIdentityMatrix, CFX_FillRenderOptions::WindingOptions()));
}

TEST(CFXRenderDeviceTest, WindowsEmfPostScriptClipBox) {
  ScopedPrintMode scoped_print_mode(WindowsPrintMode::kPostScript2);
  ScopedEnhMetaFileDC dc_handle(
      ::CreateEnhMetaFile(nullptr, nullptr, nullptr, nullptr));
  ASSERT_TRUE(dc_handle);

  const int horz_res = ::GetDeviceCaps(dc_handle.get(), HORZRES);
  const int vert_res = ::GetDeviceCaps(dc_handle.get(), VERTRES);
  const FX_RECT large_clip_rect(0, 0, horz_res * 2, vert_res * 2);

  CFX_PSFontTracker font_tracker;
  std::unique_ptr<CFX_RenderDevice> device = CFX_RenderDevice::CreateWithDriver(
      CGdiDeviceDriver::CreateDriver(dc_handle.get(), &font_tracker));
  ASSERT_TRUE(device);
  EXPECT_TRUE(device->SetClip_Rect(large_clip_rect));

  // TODO(crbug.com/553140224): Clip box should be `large_clip_rect`.
  EXPECT_EQ(FX_RECT(0, 2 * vert_res, 2 * horz_res, 0), device->GetClipBox());
}

TEST(CFXRenderDeviceTest, WindowsEmfPostScriptClipRgn) {
  ScopedPrintMode scoped_print_mode(WindowsPrintMode::kPostScript2);
  ScopedEnhMetaFileDC dc_handle(
      ::CreateEnhMetaFile(nullptr, nullptr, nullptr, nullptr));
  ASSERT_TRUE(dc_handle);

  const int horz_res = ::GetDeviceCaps(dc_handle.get(), HORZRES);
  const int vert_res = ::GetDeviceCaps(dc_handle.get(), VERTRES);
  const FX_RECT large_clip_rect(0, 0, horz_res * 2, vert_res * 2);

  ScopedGdiObject<HRGN> clip_rgn(
      ::CreateRectRgn(large_clip_rect.left, large_clip_rect.top,
                      large_clip_rect.right, large_clip_rect.bottom));
  ASSERT_TRUE(clip_rgn);
  EXPECT_NE(ERROR, ::SelectClipRgn(dc_handle.get(), clip_rgn.get()));

  CFX_PSFontTracker font_tracker;
  std::unique_ptr<CFX_RenderDevice> device = CFX_RenderDevice::CreateWithDriver(
      CGdiDeviceDriver::CreateDriver(dc_handle.get(), &font_tracker));
  ASSERT_TRUE(device);

  // TODO(crbug.com/553140224): Clip box should be `large_clip_rect`.
  EXPECT_EQ(FX_RECT(0, 2 * vert_res, 2 * horz_res, 0), device->GetClipBox());
}
#endif  // BUILDFLAG(IS_WIN)
