#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for libclang_check."""

import unittest

from libclang_check import CheckDeps


class LibclangCheckTest(unittest.TestCase):

  def testSuccess(self):
    deps = '''
deps = {
  'third_party/llvm-libclang': {
    'objects': [
      {
        'object_name': 'Linux_x64/rust-libclang-1.tar.xz',
      },
      {
        'object_name': 'Win/rust-libclang-1.tar.xz',
      },
    ],
  },
  'third_party/rust-toolchain': {
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-1.tar.xz',
      },
      {
        'object_name': 'Win/rust-toolchain-1.tar.xz',
      },
    ],
  },
}
'''
    self.assertIsNone(CheckDeps(deps))

  def testRevisionMismatch(self):
    deps = '''
deps = {
  'third_party/llvm-libclang': {
    'objects': [
      {
        'object_name': 'Linux_x64/rust-libclang-1.tar.xz',
      },
    ],
  },
  'third_party/rust-toolchain': {
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-2.tar.xz',
      },
    ],
  },
}
'''
    self.assertEqual(
        'Mismatch between rust-libclang and rust-toolchain: '
        "[('Linux_x64', '1')] vs. [('Linux_x64', '2')]", CheckDeps(deps))

  def testMissingLibclang(self):
    deps = '''
deps = {
  'third_party/rust-toolchain': {
    'objects': [
      {
        'object_name': 'Linux_x64/rust-toolchain-1.tar.xz',
      },
    ],
  },
}
'''
    self.assertEqual('Cannot parse rust-libclang revisions', CheckDeps(deps))

  def testMissingRustToolchain(self):
    deps = '''
deps = {
  'third_party/llvm-libclang': {
    'objects': [
      {
        'object_name': 'Linux_x64/rust-libclang-1.tar.xz',
      },
    ],
  },
}
'''
    self.assertEqual('Cannot parse rust-toolchain revisions', CheckDeps(deps))


if __name__ == '__main__':
  unittest.main()
