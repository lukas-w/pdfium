// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXGE_CFX_WINDOWSRENDERDEVICE_H_
#define CORE_FXGE_CFX_WINDOWSRENDERDEVICE_H_

#include <windows.h>

#include "core/fxge/cfx_renderdevice.h"

class CFX_PSFontTracker;

class CFX_WindowsRenderDevice : public CFX_RenderDevice {
 public:
  CFX_WindowsRenderDevice(HDC hDC, CFX_PSFontTracker* ps_font_tracker);
  ~CFX_WindowsRenderDevice() override;
};

#endif  // CORE_FXGE_CFX_WINDOWSRENDERDEVICE_H_
