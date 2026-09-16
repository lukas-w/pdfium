// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCRT_FX_COORDINATES_TEST_SUPPORT_H_
#define CORE_FXCRT_FX_COORDINATES_TEST_SUPPORT_H_

#include <iosfwd>

class CFX_FloatRect;
class CFX_RectF;
struct FX_RECT;

std::ostream& operator<<(std::ostream& os, const CFX_FloatRect& rect);

std::ostream& operator<<(std::ostream& os, const CFX_RectF& rect);

std::ostream& operator<<(std::ostream& os, const FX_RECT& rect);

#endif  // CORE_FXCRT_FX_COORDINATES_TEST_SUPPORT_H_
