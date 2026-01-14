#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re
import sys

ALL_DEPS_ENTRIES = (
    'abseil_revision',
    'android_toolchain_version',
    'build_revision',
    'buildtools_revision',
    'clang_format_revision',
    'clang_revision',
    'cpu_features_revision',
    'dragonbox_revision',
    'fast_float_revision',
    'fp16_revision',
    'freetype_revision',
    'gn_version',
    'gtest_revision',
    'highway_revision',
    'icu_revision',
    'jpeg_turbo_revision',
    'libcxx_revision',
    'libcxxabi_revision',
    'libunwind_revision',
    'llvm_libc_revision',
    'nasm_source_revision',
    'ninja_version',
    'partition_allocator_revision',
    'reclient_version',
    'result_adapter_revision',
    'rust_revision',
    'siso_version',
    'skia_revision',
    'testing_rust_revision',
    'tools_rust_revision',
    'tools_win_revision',
    'v8_revision',
)

UNSUPPORTED_ENTRIES = ('pdfium_tests_revision', 'test_fonts_revision')


class DepsParser:

  def __init__(self, deps_content):
    self._deps_content = deps_content
    self.deps = {}
    self.vars = {}
    # self.Str and self.Var are used by exec() below.
    self.Str = lambda s: s
    self.Var = lambda v: self.vars.get(v, f"Var('{v}')")
    exec(self._deps_content, self.__dict__)


def find_path_for_revision_in_deps(deps_dict, revision):
  """Finds the deps path that corresponds to a given revision string."""
  for path, dep_info in deps_dict.items():
    if revision in str(dep_info):
      return path
  return None


def get_revision_from_dep_value(dep_value):
  url = dep_value
  if isinstance(dep_value, dict):
    # This is for CIPD packages.
    if 'packages' in dep_value:
      for package in dep_value['packages']:
        if 'version' in package:
          return package['version']
    url = dep_value.get('url')

  if not url or not isinstance(url, str):
    return None

  match = re.search(r'@(.+)', url)
  if match:
    return match.group(1)
  return None


def roll_dep_impl(chromium_deps_parser, pdfium_deps_parser, deps_entry):
  if deps_entry in UNSUPPORTED_ENTRIES:
    return (False, f'Rolling {deps_entry} is not supported.')

  # In PDFium DEPS, only search vars for the entry.
  if deps_entry not in pdfium_deps_parser.vars:
    return (False, f'Entry "{deps_entry}" not found in PDFium DEPS.')

  pdfium_revision = pdfium_deps_parser.vars.get(deps_entry)
  deps_path = find_path_for_revision_in_deps(pdfium_deps_parser.deps,
                                             pdfium_revision)
  if not deps_path:
    return (False,
            f'Could not find path for var "{deps_entry}" in PDFium DEPS file.')

  # Check if the dependency is a CIPD package, which is not supported.
  pdfium_dep_value = pdfium_deps_parser.deps.get(deps_path)
  is_cipd = (
      isinstance(pdfium_dep_value, dict) and
      pdfium_dep_value.get('dep_type') == 'cipd')

  # In Chromium DEPS, search vars and deps.
  chromium_revision = None
  chromium_revision = chromium_deps_parser.vars.get(deps_entry)

  if not chromium_revision:
    # Not in vars, try deps.
    if deps_path:
      chromium_path = 'src/' + deps_path
      chromium_dep_value = chromium_deps_parser.deps.get(chromium_path)
      if chromium_dep_value:
        chromium_revision = get_revision_from_dep_value(chromium_dep_value)

  if not chromium_revision:
    if is_cipd:
      return (False, f'CIPD entry "{deps_entry}" not found in Chromium DEPS.')

    # If chromium_revision is still None, it means it wasn't found in Chromium
    # DEPS. In which case, suggest rolling to ToT.
    return (True, f'roll-dep {deps_path} --ignore-dirty-tree --no-log')

  if chromium_revision == pdfium_revision:
    return (True, 'Revisions are the same.')

  if is_cipd:
    return (True, f"CIPD {deps_entry}: {chromium_revision}")

  if deps_entry == 'freetype_revision':
    return (True, 'third_party/freetype/roll-freetype.sh --roll-to '
            f'{chromium_revision} --ignore-dirty-tree --no-log')

  return (True, f'roll-dep {deps_path} --roll-to {chromium_revision} '
          '--ignore-dirty-tree --no-log')


def roll_dep(chromium_deps_content, pdfium_deps_content, deps_entry):
  chromium_deps_parser = DepsParser(chromium_deps_content)
  pdfium_deps_parser = DepsParser(pdfium_deps_content)
  return roll_dep_impl(chromium_deps_parser, pdfium_deps_parser, deps_entry)


def roll_all_deps(chromium_deps_content, pdfium_deps_content):
  chromium_deps_parser = DepsParser(chromium_deps_content)
  pdfium_deps_parser = DepsParser(pdfium_deps_content)
  messages = []
  for deps_entry in ALL_DEPS_ENTRIES:
    success, message = roll_dep_impl(chromium_deps_parser, pdfium_deps_parser,
                                     deps_entry)
    if not success:
      return (False, message)

    if message == 'Revisions are the same.':
      continue

    messages.append(message)

  return (True, messages)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('chromium_deps_file', help='Path to Chromium DEPS file')
  parser.add_argument('pdfium_deps_file', help='Path to the PDFium DEPS file')
  parser.add_argument('deps_entry', help='DEPS entry to roll, can be "ALL"')
  args = parser.parse_args()

  if os.path.abspath(args.chromium_deps_file) == os.path.abspath(
      args.pdfium_deps_file):
    print('DEPS files must be different.', file=sys.stderr)
    return 1

  with open(args.chromium_deps_file, 'r') as f:
    chromium_deps_content = f.read()
  with open(args.pdfium_deps_file, 'r') as f:
    pdfium_deps_content = f.read()

  if args.deps_entry == 'ALL':
    success, maybe_messages = roll_all_deps(chromium_deps_content,
                                            pdfium_deps_content)
    if not success:
      print(maybe_messages, file=sys.stderr)
      return 1

    for message in maybe_messages:
      print(message)
    return 0

  success, message = roll_dep(chromium_deps_content, pdfium_deps_content,
                              args.deps_entry)
  if not success:
    print(message, file=sys.stderr)
    return 1

  print(message)
  return 0


if __name__ == '__main__':
  main()
