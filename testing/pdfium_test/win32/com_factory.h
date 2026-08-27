// Copyright 2023 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_PDFIUM_TEST_WIN32_COM_FACTORY_H_
#define TESTING_PDFIUM_TEST_WIN32_COM_FACTORY_H_

struct IXpsOMObjectFactory;

// Factory for COM instances.
class ComFactory final {
 public:
  ComFactory();
  ~ComFactory();

  IXpsOMObjectFactory* GetXpsOMObjectFactory();

 private:
  bool Initialize();

  bool initialized_ = false;
  IXpsOMObjectFactory* xps_om_object_factory_ = nullptr;
};

#endif  // TESTING_PDFIUM_TEST_WIN32_COM_FACTORY_H_
