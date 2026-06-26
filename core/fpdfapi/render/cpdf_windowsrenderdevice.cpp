// Copyright 2019 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/render/cpdf_windowsrenderdevice.h"

CPDF_WindowsRenderDevice::CPDF_WindowsRenderDevice(
    HDC hDC,
    CFX_PSFontTracker* ps_font_tracker)
    : CFX_WindowsRenderDevice(hDC, ps_font_tracker) {}

CPDF_WindowsRenderDevice::~CPDF_WindowsRenderDevice() = default;
