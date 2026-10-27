// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/fxcrt/scoped_locale.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
#include <locale.h>
#include "core/fxcrt/check_op.h"
#endif

namespace pdfium {

ScopedLocale::ScopedLocale(const std::string& locale) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
  prev_locale_ = setlocale(LC_ALL, nullptr);
  CHECK(setlocale(LC_ALL, locale.c_str()) != nullptr);
#endif
}

ScopedLocale::~ScopedLocale() {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
  CHECK_EQ(prev_locale_, setlocale(LC_ALL, prev_locale_.c_str()));
#endif
}

}  // namespace pdfium
