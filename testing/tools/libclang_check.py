#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Verifies rust-libclang and rust-toolchain revisions are in sync in DEPS."""

import re
import sys


def _ExtractRevisions(deps_content, regex):
  """Gets the revisions by searching the lines in deps_content using regex."""
  revisions = []
  for line in deps_content.splitlines():
    match = regex.match(line)
    if not match:
      continue
    revisions.append((match.group(1), match.group(2)))
  return revisions


def _GetLibclangRevisions(deps_content):
  """Gets the rust-libclang revisions from DEPS content."""
  regex = re.compile(
      r"^        'object_name': '(.*)/rust-libclang-(.*)\.tar\.xz',$")
  return _ExtractRevisions(deps_content, regex)


def _GetRustToolchainRevisions(deps_content):
  """Gets the rust-toolchain revisions from DEPS content."""
  regex = re.compile(
      r"^        'object_name': '(.*)/rust-toolchain-(.*)\.tar\.xz',$")
  return _ExtractRevisions(deps_content, regex)


def CheckDeps(deps_content):
  """Checks that rust-libclang and rust-toolchain revisions match in DEPS."""
  libclang_revisions = _GetLibclangRevisions(deps_content)
  if not libclang_revisions:
    return 'Cannot parse rust-libclang revisions'

  rust_revisions = _GetRustToolchainRevisions(deps_content)
  if not rust_revisions:
    return 'Cannot parse rust-toolchain revisions'

  if libclang_revisions != rust_revisions:
    return ('Mismatch between rust-libclang and rust-toolchain: '
            f'{libclang_revisions} vs. {rust_revisions}')
  return None


def main():
  if len(sys.argv) != 2:
    print('Wrong number of arguments')
    return 0

  deps_path = sys.argv[1]
  with open(deps_path, 'r', encoding='utf-8') as f:
    deps_content = f.read()

  error = CheckDeps(deps_content)
  if error:
    print(f'{deps_path}: {error}')
  return 0


if __name__ == '__main__':
  sys.exit(main())
