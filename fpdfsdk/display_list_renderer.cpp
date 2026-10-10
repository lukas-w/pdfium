// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "fpdfsdk/display_list_renderer.h"

#include <utility>

#include "core/fpdfapi/page/cpdf_occontext.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/render/cpdf_pagerendercontext.h"
#include "core/fpdfapi/render/cpdf_renderoptions.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/display_list_device_driver.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/cpdfsdk_pageview.h"
#include "fpdfsdk/cpdfsdk_renderpage.h"
#include "public/fpdfview.h"

// static
std::string DisplayListRenderer::RenderPage(FPDF_PAGE page,
                                            FPDF_FORMHANDLE form,
                                            int flags) {
  int width = static_cast<int>(FPDF_GetPageWidthF(page));
  int height = static_cast<int>(FPDF_GetPageHeightF(page));
  DisplayListRenderer renderer(page, width, height, flags);
  if (!renderer.Start()) {
    return std::string();
  }
  renderer.Finish(form);
  return renderer.GetDisplayList();
}

DisplayListRenderer::DisplayListRenderer(FPDF_PAGE page,
                                         int width,
                                         int height,
                                         int flags)
    : page_(page), width_(width), height_(height), flags_(flags) {
  auto driver = std::make_unique<DisplayListDeviceDriver>(width, height);
  driver_ = driver.get();
  device_ = CFX_RenderDevice::CreateWithDriver(std::move(driver));
}

DisplayListRenderer::~DisplayListRenderer() = default;

bool DisplayListRenderer::Start() {
  if (!device_) {
    return false;
  }
  CPDF_Page* cpdf_page = CPDFPageFromFPDFPage(page_);
  if (!cpdf_page) {
    return false;
  }
  context_ = std::make_unique<CPDF_PageRenderContext>();
  context_->device_ = std::move(device_);
  CPDFSDK_RenderPageWithContext(context_.get(), cpdf_page, /*start_x=*/0,
                                /*start_y=*/0, /*size_x=*/width_,
                                /*size_y=*/height_, /*rotate=*/0,
                                /*flags=*/flags_, nullptr,
                                /*need_to_restore=*/true, nullptr);
  return true;
}

void DisplayListRenderer::Finish(FPDF_FORMHANDLE form) {
  if (form && context_ && context_->device_) {
    CPDFSDK_FormFillEnvironment* form_fill_env =
        CPDFSDKFormFillEnvironmentFromFPDFFormHandle(form);
    IPDF_Page* ipdf_page = IPDFPageFromFPDFPage(page_);
    if (form_fill_env && ipdf_page) {
      CPDFSDK_PageView* page_view =
          form_fill_env->GetOrCreatePageView(ipdf_page);
      if (page_view) {
        CPDF_Page* cpdf_page = CPDFPageFromFPDFPage(page_);
        const FX_RECT rect(0, 0, width_, height_);
        CFX_Matrix matrix = cpdf_page->GetDisplayMatrixForRect(rect, 0);
        CPDF_RenderOptions options;
        options.GetOptions().bClearType = !!(flags_ & FPDF_LCD_TEXT);
        if (flags_ & FPDF_GRAYSCALE) {
          options.SetColorMode(CPDF_RenderOptions::kGray);
        }
        options.SetDrawAnnots(flags_ & FPDF_ANNOT);
        options.SetOCContext(pdfium::MakeRetain<CPDF_OCContext>(
            cpdf_page->GetDocument(), CPDF_OCContext::kView));
        page_view->PageView_OnDraw(context_->device_.get(), matrix, &options,
                                   rect);
      }
    }
  }
}

std::string DisplayListRenderer::GetDisplayList() const {
  return std::string(driver_->display_list().c_str());
}
