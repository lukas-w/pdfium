// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_CSS_CFX_CSSCOLORVALUE_H_
#define CORE_FXCRT_CSS_CFX_CSSCOLORVALUE_H_

#include <stdint.h>

#include "core/fxcrt/css/cfx_cssvalue.h"

// ARGB color packed as 0xAARRGGBB.
using CFX_CSSColor = uint32_t;

// Explicit ARGB byte packing avoids an upward layering dependency on FX_ARGB
// and fxge pixel-packing functions/macros (such as ArgbEncode()).
constexpr CFX_CSSColor CFX_CSSColorPack(uint8_t a,
                                        uint8_t r,
                                        uint8_t g,
                                        uint8_t b) {
  return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
         (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}

class CFX_CSSColorValue final : public CFX_CSSValue {
 public:
  explicit CFX_CSSColorValue(CFX_CSSColor color);
  ~CFX_CSSColorValue() override;

  CFX_CSSColor Value() const { return value_; }

 private:
  CFX_CSSColor value_;
};

#endif  // CORE_FXCRT_CSS_CFX_CSSCOLORVALUE_H_
