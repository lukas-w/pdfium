#!/usr/bin/env python3
# Copyright 2015 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common
import pngdiffer


def _ParseExtraOptions(tokens):
  flags = []
  for token in tokens:
    if not token.startswith('fuzzy='):
      raise ValueError(f'Unexpected option in suppressions: {token}')
    params = token[len('fuzzy='):].split(',')
    if not (1 <= len(params) <= 3):
      raise ValueError(f'Invalid fuzzy option format: {token}')
    try:
      delta = int(params[0])
    except ValueError as e:
      raise ValueError(f'Invalid delta value in fuzzy option: {token}') from e
    if not (0 <= delta <= 255):
      raise ValueError(
          f'Delta value {delta} out of range [0, 255] in fuzzy option: {token}')
    for p in params[1:]:
      try:
        val = float(p)
      except ValueError as e:
        raise ValueError(
            f'Invalid float value "{p}" in fuzzy option: {token}') from e
      if val < 0.0:
        raise ValueError(
            f'Negative value {val} not allowed in fuzzy option: {token}')
    flags.append(f'--{token}')
  return flags


class Suppressor:

  def __init__(self, finder, features, js_disabled, xfa_disabled,
               rendering_option):
    self.has_v8 = not js_disabled and 'V8' in features
    self.has_xfa = not js_disabled and not xfa_disabled and 'XFA' in features
    self.has_rust_decoders = ('RUST_BMP' in features or
                              'RUST_JPEG' in features or 'RUST_PNG' in features)
    self.rendering_option = rendering_option
    self.suppression_set = self._LoadSuppressedSet('SUPPRESSIONS', finder)
    self.image_suppression_set = self._LoadSuppressedSet(
        'SUPPRESSIONS_IMAGE_DIFF', finder)
    self.exact_matching_suppression_dict = self._LoadSuppressedDict(
        'SUPPRESSIONS_EXACT_MATCHING', finder, has_value_column=True)

  def _LoadSuppressedSet(self, suppressions_filename, finder):
    return set(
        self._LoadSuppressedDict(
            suppressions_filename, finder, has_value_column=False).keys())

  def _LoadSuppressedDict(self,
                          suppressions_filename,
                          finder,
                          has_value_column=False):
    v8_option = "v8" if self.has_v8 else "nov8"
    xfa_option = "xfa" if self.has_xfa else "noxfa"
    with open(os.path.join(finder.TestingDir(), suppressions_filename)) as f:
      os_name = common.os_name()
      mac_platform = common.mac_platform() if os_name == 'mac' else None
      result = {}
      for item in self._ExtractSuppressions(f):
        assert len(item) >= 5 if has_value_column else len(item) == 5, (
            f'Unexpected column count in {suppressions_filename}: {item}')
        if self._MatchSuppression(item, os_name, mac_platform, v8_option,
                                  xfa_option, self.rendering_option):
          filename = item[0]
          result[filename] = _ParseExtraOptions(
              item[5:]) if has_value_column else []
      return result

  def _ExtractSuppressions(self, f):
    return [
        y.split() for y in [x.split('#')[0].strip() for x in f.readlines()] if y
    ]

  @staticmethod
  def _MatchOs(os_name, mac_platform, os_column):
    if '*' in os_column or os_name in os_column:
      return True
    if os_name == 'mac':
      assert mac_platform
      return f'{os_name}_{mac_platform}' in os_column
    return False

  def _MatchSuppression(self, item, os_name, mac_platform, js, xfa,
                        rendering_option):
    os_column = item[1].split(",")
    js_column = item[2].split(",")
    xfa_column = item[3].split(",")
    rendering_option_column = item[4].split(",")
    return (Suppressor._MatchOs(os_name, mac_platform, os_column) and
            ('*' in js_column or js in js_column) and
            ('*' in xfa_column or xfa in xfa_column) and
            ('*' in rendering_option_column or
             rendering_option in rendering_option_column))

  def IsResultSuppressed(self, input_filename):
    if input_filename in self.suppression_set:
      print("%s result is suppressed" % input_filename)
      return True
    return False

  def IsExecutionSuppressed(self, input_filepath):
    if "xfa_specific" in input_filepath and not self.has_xfa:
      print("%s execution is suppressed" % input_filepath)
      return True
    return False

  def IsImageDiffSuppressed(self, input_filename):
    if input_filename in self.image_suppression_set:
      print("%s image diff comparison is suppressed" % input_filename)
      return True
    return False

  def GetImageMatchingAlgorithm(self, input_filename):
    if input_filename in self.exact_matching_suppression_dict:
      print(f"{input_filename} image diff comparison is fuzzy")
      return (pngdiffer.FUZZY_MATCHING,
              self.exact_matching_suppression_dict[input_filename])
    if self.has_rust_decoders:
      print(f"{input_filename} image diff comparison is fuzzy")
      return (pngdiffer.FUZZY_MATCHING, [])
    return (pngdiffer.EXACT_MATCHING, [])
