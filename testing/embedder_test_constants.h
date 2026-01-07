// Copyright 2020 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_EMBEDDER_TEST_CONSTANTS_H_
#define TESTING_EMBEDDER_TEST_CONSTANTS_H_

namespace pdfium {

// MD5 hash for rendering annotation_stamp_with_ap.pdf with annotations.
const char* AnnotationStampWithApChecksum();

// Expectation file basename for rendering a 200x200 blank page.
inline constexpr char kBlankPage200x200Png[] = "blank_200x200";

// Expectation file basename for rendering a 612x792 blank page.
inline constexpr char kBlankPage612By792Png[] = "blank_612x792";

// MD5 hash for rendering bug_890322.pdf.
const char* Bug890322Checksum();

// Expectation file basename for rendering hello_world.pdf or bug_455199.pdf.
inline constexpr char kHelloWorldPng[] = "hello_world";

// MD5 hash for rendering hello_world.pdf after removing "Goodbye, world!".
const char* HelloWorldRemovedChecksum();

// MD5 hash for rendering many_rectangles.pdf.
const char* ManyRectanglesChecksum();

// MD5 hash for rendering rectangles.pdf.
const char* RectanglesChecksum();

// MD5 hash for rendering text_form.pdf.
const char* TextFormChecksum();

}  // namespace pdfium

#endif  // TESTING_EMBEDDER_TEST_CONSTANTS_H_
