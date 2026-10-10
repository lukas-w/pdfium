// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FPDFSDK_DISPLAY_LIST_RENDERER_H_
#define FPDFSDK_DISPLAY_LIST_RENDERER_H_

#include <memory>
#include <string>

#include "core/fxcrt/unowned_ptr.h"
#include "public/fpdfview.h"

class CFX_RenderDevice;
class CPDF_PageRenderContext;
class DisplayListDeviceDriver;

class DisplayListRenderer {
 public:
  // Convenience method for one-shot synchronous rendering of a page.
  static std::string RenderPage(FPDF_PAGE page,
                                FPDF_FORMHANDLE form,
                                int flags);

  DisplayListRenderer(FPDF_PAGE page, int width, int height, int flags);
  ~DisplayListRenderer();

  bool Start();
  void Finish(FPDF_FORMHANDLE form);

  std::string GetDisplayList() const;

 private:
  FPDF_PAGE const page_;
  const int width_;
  const int height_;
  const int flags_;
  std::unique_ptr<CPDF_PageRenderContext> context_;  // must outlive `driver_`
  std::unique_ptr<CFX_RenderDevice> device_;         // must outlive `driver_`
  UnownedPtr<DisplayListDeviceDriver> driver_;
};

#endif  // FPDFSDK_DISPLAY_LIST_RENDERER_H_
