// Copyright 2022 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/command_line_helpers.h"

bool ParseSwitchKeyValue(const std::string& arg,
                         const std::string& key,
                         std::string* value) {
  if (arg.size() <= key.size() || arg.compare(0, key.size(), key) != 0) {
    return false;
  }

  *value = arg.substr(key.size());
  return true;
}

FPDF_RENDERER_TYPE GetDefaultRendererType() {
#if defined(PDF_USE_SKIA)
  return FPDF_RENDERERTYPE_SKIA;
#else
  return FPDF_RENDERERTYPE_AGG;
#endif
}

FPDF_FONT_BACKEND_TYPE GetDefaultFontLibraryType() {
#if defined(PDF_ENABLE_FREETYPE)
  return FPDF_FONTBACKENDTYPE_FREETYPE;
#else
  return FPDF_FONTBACKENDTYPE_FONTATIONS;
#endif
}
