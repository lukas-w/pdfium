// Copyright 2024 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FXCRT_SCOPED_LOCALE_H_
#define CORE_FXCRT_SCOPED_LOCALE_H_

#include <string>

namespace pdfium {

// On certain platforms, sets the given `locale` on construction, and restores
// the previous locale on destruction.
class ScopedLocale {
 public:
  explicit ScopedLocale(const std::string& locale);
  ~ScopedLocale();

 private:
  std::string prev_locale_;

  ScopedLocale(const ScopedLocale&) = delete;
  ScopedLocale& operator=(const ScopedLocale&) = delete;
};

}  // namespace pdfium

#endif  // CORE_FXCRT_SCOPED_LOCALE_H_
