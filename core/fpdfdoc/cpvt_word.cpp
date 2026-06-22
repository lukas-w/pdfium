// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fpdfdoc/cpvt_word.h"

CPVT_Word::CPVT_Word() = default;

CPVT_Word::CPVT_Word(uint16_t word,
                     FX_Charset charset,
                     const CFX_PointF& location,
                     float ascent,
                     float descent,
                     float width,
                     int32_t font_index,
                     float font_size)
    : word_(word),
      charset_(charset),
      location_(location),
      ascent_(ascent),
      descent_(descent),
      width_(width),
      font_index_(font_index),
      font_size_(font_size) {}

CPVT_Word::CPVT_Word(const CPVT_Word&) = default;
CPVT_Word& CPVT_Word::operator=(const CPVT_Word&) = default;
CPVT_Word::~CPVT_Word() = default;
