#!/usr/bin/env python3
# Copyright 2015 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import common
import pngdiffer


def _ParseFuzzyAction(token):
  """Converts a fuzzy action token into a single pdfium_diff flag."""
  if token == 'fuzzy':
    return '--fuzzy'
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
  return f'--{token}'


# Legal values for each predicate column, keyed by column index. Column 0 is
# the test file name, which is unconstrained.
_VALID_COLUMN_VALUES = {
    1: {'*', 'win', 'mac', 'mac_arm', 'mac_x86', 'linux'},
    2: {'*', 'nov8', 'v8'},
    3: {'*', 'noxfa', 'xfa'},
    4: {'*', 'agg', 'gdi', 'gdi_agg', 'gdi_skia', 'skia'},
    5: {'*', 'freetype', 'fontations'},
}

# Legal keywords for the action in column 6.
_VALID_ACTIONS = {'diff', 'blank', 'fuzzy', 'skip'}


def _ValidatePredicates(item):
  """Rejects unknown tokens, which would otherwise silently never match."""
  for column, valid_values in _VALID_COLUMN_VALUES.items():
    for value in item[column].split(','):
      if value not in valid_values:
        raise ValueError(f'Unexpected value "{value}" in column {column} of '
                         f'suppressions: {" ".join(item)}')


def _ParseAction(token):
  """Validates an action token, returning its keyword and any diff flags."""
  keyword = token.split('=', 1)[0]
  if keyword not in _VALID_ACTIONS:
    raise ValueError(f'Unexpected action in suppressions: {token}')
  return keyword, ([_ParseFuzzyAction(token)] if keyword == 'fuzzy' else [])


def _SuppressionSpecificity(item):
  """Scores columns 1-5 as a little-endian specificity bitmask."""
  return sum((item[col] != '*') << (col - 1) for col in range(1, 6))


class Suppressor:

  def __init__(self, finder, features, js_disabled, xfa_disabled,
               rendering_option, font_engine):
    self.has_v8 = not js_disabled and 'V8' in features
    self.has_xfa = not js_disabled and not xfa_disabled and 'XFA' in features
    if rendering_option == 'gdi':
      self.rendering_option = 'gdi_skia' if 'SKIA' in features else 'gdi_agg'
    else:
      self.rendering_option = rendering_option
    self.font_engine = font_engine
    self.suppression_set = set()
    self.execution_suppression_set = set()
    self.image_suppression_set = set()
    self.exact_matching_suppression_dict = {}
    self._LoadSuppressions(finder)

  def _LoadSuppressions(self, finder):
    v8_option = "v8" if self.has_v8 else "nov8"
    xfa_option = "xfa" if self.has_xfa else "noxfa"
    with open(os.path.join(finder.TestingDir(), 'SUPPRESSIONS')) as f:
      os_name = common.os_name()
      mac_platform = common.mac_platform() if os_name == 'mac' else None
      fuzzy_specificity = {}
      for item in self._ExtractSuppressions(f):
        if len(item) != 7:
          raise ValueError(f'Unexpected column count in suppressions: {item}')
        _ValidatePredicates(item)
        keyword, flags = _ParseAction(item[6])
        if not self._MatchSuppression(item, os_name, mac_platform, v8_option,
                                      xfa_option, self.rendering_option,
                                      self.font_engine):
          continue
        filename = item[0]
        if keyword == 'diff':
          self.suppression_set.add(filename)
        elif keyword == 'skip':
          self.execution_suppression_set.add(filename)
        elif keyword == 'blank':
          self.image_suppression_set.add(filename)
        elif keyword == 'fuzzy':
          specificity = _SuppressionSpecificity(item)
          if specificity >= fuzzy_specificity.get(filename, -1):
            fuzzy_specificity[filename] = specificity
            self.exact_matching_suppression_dict[filename] = flags
        else:
          raise AssertionError(f'Unhandled action: {keyword}')

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

  @staticmethod
  def _MatchRenderer(rendering_option, rendering_option_column):
    if ('*' in rendering_option_column or
        rendering_option in rendering_option_column):
      return True
    if rendering_option.startswith('gdi_'):
      return 'gdi' in rendering_option_column
    return False

  def _MatchSuppression(self, item, os_name, mac_platform, js, xfa,
                        rendering_option, font_engine):
    os_column = item[1].split(",")
    js_column = item[2].split(",")
    xfa_column = item[3].split(",")
    rendering_option_column = item[4].split(",")
    font_engine_column = item[5].split(",")
    return (Suppressor._MatchOs(os_name, mac_platform, os_column) and
            ('*' in js_column or js in js_column) and
            ('*' in xfa_column or xfa in xfa_column) and
            Suppressor._MatchRenderer(rendering_option,
                                      rendering_option_column) and
            ('*' in font_engine_column or font_engine in font_engine_column))

  def IsResultSuppressed(self, input_filename):
    if input_filename in self.suppression_set:
      print("%s result is suppressed" % input_filename)
      return True
    return False

  def IsExecutionSuppressed(self, input_filepath):
    if (('xfa_specific' in input_filepath and not self.has_xfa) or
        os.path.basename(input_filepath) in self.execution_suppression_set):
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
    return (pngdiffer.EXACT_MATCHING, [])
