// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXGE_CFX_STANDARDFONT_H_
#define CORE_FXGE_CFX_STANDARDFONT_H_

#include <stdint.h>

#include <optional>

#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/span.h"

class CFX_StandardFont {
 public:
  enum StandardFont : uint8_t {
    kCourier = 0,
    kCourierBold,
    kCourierBoldOblique,
    kCourierOblique,
    kHelvetica,
    kHelveticaBold,
    kHelveticaBoldOblique,
    kHelveticaOblique,
    kTimes,
    kTimesBold,
    kTimesBoldOblique,
    kTimesOblique,
    kSymbol,
    kDingbats,
    kLast = kDingbats
  };
  static constexpr int kNumStandardFonts = 14;

  static std::optional<StandardFont> GetStandardFontIndex(
      const ByteString& name);
  static ByteString GetCanonicalFontName(StandardFont font);

  static bool IsStandardFontName(const ByteString& name);
  static bool IsSymbolicFont(StandardFont font);
  static bool IsFixedFont(StandardFont font);

  static pdfium::span<const uint8_t> GetFontData(StandardFont font);
  static pdfium::span<const uint8_t> GetGenericSansFontData();
  static pdfium::span<const uint8_t> GetGenericSerifFontData();
};

#endif  // CORE_FXGE_CFX_STANDARDFONT_H_
