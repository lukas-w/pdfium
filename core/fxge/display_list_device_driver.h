// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_DISPLAY_LIST_DEVICE_DRIVER_H_
#define CORE_FXGE_DISPLAY_LIST_DEVICE_DRIVER_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/span.h"
#include "core/fxge/renderdevicedriver_iface.h"

class CFX_DIBBase;
class CFX_DIBitmap;
class CFX_Font;
class CFX_GraphStateData;
class CFX_Matrix;
class CFX_Path;
struct CFX_FillRenderOptions;
struct CFX_TextRenderOptions;
struct FX_RECT;
struct TextCharPos;

class DisplayListDeviceDriver final : public RenderDeviceDriverIface {
 public:
  DisplayListDeviceDriver(int width, int height);
  ~DisplayListDeviceDriver() override;

  const ByteString& display_list() const { return display_list_; }
  void ClearDisplayList() { display_list_.clear(); }

  // RenderDeviceDriverIface:
  DeviceType GetDeviceType() const override;
  int GetPixelWidth() const override;
  int GetPixelHeight() const override;
  int GetBitsPerPixel() const override;
  bool RenderCapAlphaOutput() const override;
#if defined(PDF_USE_SKIA)
  bool RenderCapFillStrokePath() const override;
#endif

  void Clear(uint32_t color) override;
  void SaveState() override;
  void RestoreState(bool bKeepSaved) override;

  void SetBaseClip(const FX_RECT& rect) override;
  bool SetClip_PathFill(const CFX_Path& path,
                        const CFX_Matrix* pObject2Device,
                        const CFX_FillRenderOptions& fill_options) override;
  bool SetClip_PathStroke(const CFX_Path& path,
                          const CFX_Matrix* pObject2Device,
                          const CFX_GraphStateData* pGraphState) override;
  bool DrawPath(const CFX_Path& path,
                const CFX_Matrix* pObject2Device,
                const CFX_GraphStateData* pGraphState,
                uint32_t fill_color,
                uint32_t stroke_color,
                bool group_knockout,
                const CFX_FillRenderOptions& fill_options) override;
  bool FillRect(const FX_RECT& rect, uint32_t fill_color) override;
  bool DrawCosmeticLine(const CFX_PointF& ptMoveTo,
                        const CFX_PointF& ptLineTo,
                        uint32_t color) override;

  FX_RECT GetClipBox() const override;
  bool SetDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                 uint32_t color,
                 const FX_RECT& src_rect,
                 int left,
                 int top,
                 BlendMode blend_type) override;
  bool StretchDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                     uint32_t color,
                     int dest_left,
                     int dest_top,
                     int dest_width,
                     int dest_height,
                     const FX_RECT* pClipRect,
                     const FXDIB_ResampleOptions& options,
                     BlendMode blend_type) override;
  StartResult StartDIBits(RetainPtr<const CFX_DIBBase> bitmap,
                          float alpha,
                          uint32_t color,
                          const CFX_Matrix& matrix,
                          const FXDIB_ResampleOptions& options,
                          BlendMode blend_type) override;
  bool DrawDeviceText(pdfium::span<const TextCharPos> pCharPos,
                      CFX_Font* font,
                      const CFX_Matrix& mtObject2Device,
                      float font_size,
                      uint32_t color,
                      const CFX_TextRenderOptions& options) override;
  bool MultiplyAlpha(float alpha) override;
  bool MultiplyAlphaMask(RetainPtr<const CFX_DIBitmap> mask) override;

 private:
  const int width_;
  const int height_;
  FX_RECT clip_box_;
  std::vector<FX_RECT> clip_stack_;
  ByteString display_list_;
};

#endif  // CORE_FXGE_DISPLAY_LIST_DEVICE_DRIVER_H_
