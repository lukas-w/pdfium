// Copyright 2017 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/pdf_test_environment.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include <dbghelp.h>
#endif

#if defined(PDF_USE_PARTITION_ALLOC)
#include "testing/allocator_shim_config.h"
#endif

#ifdef PDF_ENABLE_V8
#include "testing/v8_test_environment.h"
#ifdef PDF_ENABLE_XFA
#include "testing/xfa_test_environment.h"
#endif  // PDF_ENABLE_XFA
#endif  // PDF_ENABLE_V8

#if BUILDFLAG(IS_WIN)
namespace {

// In SDK 10.0.28000+, dbghelp.dll dynamically loads msdia140.dll at runtime,
// and symbolization calls WinVerifyTrust() which creates threads in crypt32.
// If dbghelp is initialized during ASAN symbolization of a crash, this
// causes reentrancy or deadlocks inside AddressSanitizer's thread registry
// or malloc handlers (see crbug.com/548509159). Therefore, preload these
// DLLs and initialize DbgHelp before test execution starts.
void PreloadSymbols() {
  ::LoadLibraryW(L"dbghelp.dll");
  ::LoadLibraryW(L"msdia140.dll");
  if (::SymInitialize(::GetCurrentProcess(), nullptr, TRUE)) {
    ::SymCleanup(::GetCurrentProcess());
  }
}

}  // namespace
#endif  // BUILDFLAG(IS_WIN)

// Can't use gtest-provided main since we need to initialize partition
// alloc before invoking any test, and add test environments.
int main(int argc, char** argv) {
#if defined(PDF_USE_PARTITION_ALLOC)
  pdfium::ConfigurePartitionAllocShimPartitionForTest();
#endif  // defined(PDF_USE_PARTITION_ALLOC)

#if BUILDFLAG(IS_WIN)
  PreloadSymbols();
#endif  // BUILDFLAG(IS_WIN)

  // PDF test environment will be deleted by gtest.
  auto* pdf_test_environment = new PDFTestEnvironment();
  AddGlobalTestEnvironment(pdf_test_environment);

#ifdef PDF_ENABLE_V8
  // V8 test environment will be deleted by gtest.
  AddGlobalTestEnvironment(new V8TestEnvironment(argv[0]));
#ifdef PDF_ENABLE_XFA
  // XFA test environment will be deleted by gtest.
  AddGlobalTestEnvironment(new XFATestEnvironment());
#endif  // PDF_ENABLE_XFA
#endif  // PDF_ENABLE_V8

  testing::InitGoogleTest(&argc, argv);
  testing::InitGoogleMock(&argc, argv);

  // Anything remaining in argc/argv is a unit_tests flag.
  pdf_test_environment->AddFlags(argc, argv);

  return RUN_ALL_TESTS();
}
