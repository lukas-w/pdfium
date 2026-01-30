// Copyright 2016 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fxge/cfx_fontcache.h"

#include "core/fxge/cfx_font.h"
#include "core/fxge/cfx_glyphcache.h"
#include "core/fxge/fx_font.h"

CFX_FontCache::CFX_FontCache() = default;

CFX_FontCache::~CFX_FontCache() = default;

RetainPtr<CFX_GlyphCache> CFX_FontCache::GetGlyphCache(const CFX_Font* font) {
  RetainPtr<CFX_Face> face = font->GetFace();
  auto it = glyph_cache_map_.find(face.Get());
  if (it != glyph_cache_map_.end() && it->second) {
    return pdfium::WrapRetain(it->second.Get());
  }
  auto new_cache = pdfium::MakeRetain<CFX_GlyphCache>(face);
  glyph_cache_map_[face.Get()].Reset(new_cache.Get());
  return new_cache;
}
