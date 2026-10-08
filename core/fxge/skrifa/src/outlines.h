// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Based on https://github.com/googlefonts/fontations/pull/1820

#pragma once
#include "third_party/rust/chromium_crates_io/vendor/cxx-v1/include/cxx.h"
namespace skrifa {
void run(rust::Str font_path);
}
