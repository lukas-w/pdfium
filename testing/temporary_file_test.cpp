// Copyright 2026 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/temporary_file_test.h"

#include <fcntl.h>
#include <unistd.h>

TemporaryFileTest::TemporaryFileTest() = default;

TemporaryFileTest::~TemporaryFileTest() = default;

void TemporaryFileTest::SetUp() {
  fd_ = mkstemp(temp_name_);
  ASSERT_GE(fd_, 0);
}

void TemporaryFileTest::TearDown() {
  if (fd_ != -1) {
    close(fd_);
  }
  unlink(temp_name_);
}

void TemporaryFileTest::WriteAndClose(pdfium::span<const uint8_t> data) {
  if (!data.empty()) {
    (void)write(fd_, data.data(), data.size());
  }
  close(fd_);
  fd_ = -1;
}
