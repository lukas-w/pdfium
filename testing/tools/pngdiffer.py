#!/usr/bin/env python3
# Copyright 2015 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from dataclasses import dataclass
import itertools
import os
import shutil
import subprocess

EXACT_MATCHING = 'exact'
FUZZY_MATCHING = 'fuzzy'

_PNG_OPTIMIZER = 'optipng'

# Each suffix order acts like a path along a tree, with the leaves being the
# most specific, and the root being the least specific.
_COMMON_SUFFIX_ORDER = ('_{os}', '')
_AGG_SUFFIX_ORDER = ('_agg_{os}', '_agg') + _COMMON_SUFFIX_ORDER
_GDI_AGG_SUFFIX_ORDER = ('_gdi',) + _COMMON_SUFFIX_ORDER
_GDI_SKIA_SUFFIX_ORDER = ('_skia_gdi', '_gdi') + _COMMON_SUFFIX_ORDER
_SKIA_SUFFIX_ORDER = ('_skia_{os}', '_skia') + _COMMON_SUFFIX_ORDER


@dataclass
class DiffMetrics:
  actual_w: int
  actual_h: int
  expected_w: int
  expected_h: int
  pixels: int
  total: int
  max_delta: int
  mse: float
  win_mse: float
  c_result: str

  @property
  def dims_match(self):
    return self.actual_w == self.expected_w and self.actual_h == self.expected_h


@dataclass
class ImageDiff:
  """Details about an image diff.

  Attributes:
    actual_path: Path to the actual image file.
    expected_path: Path to the expected image file, or `None` if no matches.
    diff_path: Path to the diff image file, or `None` if no diff.
    reason: Optional reason for the diff.
    metrics: Optional DiffMetrics from comparison.
    comparison_failure: Whether the diff came from comparing images rather
        than a tool execution failure.
  """

  actual_path: str
  expected_path: str = None
  diff_path: str = None
  reason: str = None
  metrics: DiffMetrics = None
  comparison_failure: bool = True


class PNGDiffer:

  def __init__(self, finder, reverse_byte_order, rendering_option,
               default_renderer):
    self.pdfium_diff_path = finder.ExecutablePath('pdfium_diff')
    self.os_name = finder.os_name
    self.reverse_byte_order = reverse_byte_order

    self.suffix_order = None
    if rendering_option == 'gdi':
      if default_renderer == 'agg':
        self.suffix_order = _GDI_AGG_SUFFIX_ORDER
      elif default_renderer == 'skia':
        self.suffix_order = _GDI_SKIA_SUFFIX_ORDER
    elif rendering_option == 'agg':
      self.suffix_order = _AGG_SUFFIX_ORDER
    elif rendering_option == 'skia':
      self.suffix_order = _SKIA_SUFFIX_ORDER

    if not self.suffix_order:
      raise ValueError(f'rendering_option={rendering_option}')

  def CheckMissingTools(self, regenerate_expected):
    if regenerate_expected and not shutil.which(_PNG_OPTIMIZER):
      return f'Please install "{_PNG_OPTIMIZER}" to regenerate expected images.'
    return None

  def GetActualFiles(self, input_filename, source_dir, working_dir):
    actual_paths = []
    path_templates = _PathTemplates(input_filename, source_dir, working_dir,
                                    self.os_name, self.suffix_order)

    for page in itertools.count():
      actual_path = path_templates.GetActualPath(page)
      if path_templates.GetExpectedPath(page, default_to_base=False):
        actual_paths.append(actual_path)
      else:
        break
    return actual_paths

  def _RunCommand(self, cmd):
    try:
      subprocess.run(cmd, capture_output=True, check=True)
      return None
    except subprocess.CalledProcessError as e:
      return e

  def _ParseTolerances(self, algorithm, extra_flags):
    if algorithm != FUZZY_MATCHING:
      return None
    # These duplicate kMaxFuzzy* in testing/utils/pixel_diff_util.h. Change one
    # without the other and fuzzy tests silently judge by the wrong limits.
    delta = 3
    mse = 0.05
    win_mse = 15.0
    for flag in extra_flags:
      if flag.startswith('--fuzzy='):
        params = flag[len('--fuzzy='):].split(',')
        if len(params) > 0 and params[0]:
          delta = int(params[0])
        if len(params) > 1 and params[1]:
          mse = float(params[1])
        if len(params) > 2 and params[2]:
          win_mse = float(params[2])
    return delta, mse, win_mse

  def EvaluateMatch(self, metrics, image_matching_algorithm):
    if not metrics.dims_match:
      return False, (
          f'dimension mismatch: actual {metrics.actual_w}x{metrics.actual_h} '
          f'!= expected {metrics.expected_w}x{metrics.expected_h}')

    algorithm = image_matching_algorithm
    extra_flags = []
    if isinstance(image_matching_algorithm, (tuple, list)):
      algorithm, extra_flags = image_matching_algorithm

    if algorithm == EXACT_MATCHING:
      if metrics.pixels == 0:
        return True, None
      return False, (
          f'diff: {metrics.pixels} pixels, max_delta={metrics.max_delta}, '
          f'mse={metrics.mse:.6f}')

    if algorithm == FUZZY_MATCHING:
      delta, mse, win_mse = self._ParseTolerances(algorithm, extra_flags)
      if (metrics.max_delta <= delta and (mse <= 0.0 or metrics.mse <= mse) and
          (win_mse <= 0.0 or metrics.win_mse <= win_mse)):
        return True, None
      return False, (
          f'exceeds fuzzy: max_delta={metrics.max_delta} (max {delta}), '
          f'mse={metrics.mse:.6f} (max {mse})')

    return False, f'Unknown algorithm {algorithm}'

  # TODO(tsepez): Remove along with the shadow validation once the Python
  # decision logic is trusted.
  def _CheckMetricsParity(self, cmd, metrics_stdout, metrics_line, c_pass):
    """Verifies that `--metrics` leaves the legacy comparison untouched."""
    legacy_cmd = [arg for arg in cmd if arg != '--metrics']
    legacy = subprocess.run(
        legacy_cmd, capture_output=True, text=True, check=False)

    # Without `--metrics`, the result comes back as the exit code, and the
    # output is the metrics output less the metrics line itself.
    assert (legacy.returncode == 0) == c_pass, (
        f'Metrics parity mismatch on {legacy_cmd}!\n'
        f'Exit code {legacy.returncode} disagrees with c_result '
        f'({"PASS" if c_pass else "FAIL"})')
    assert legacy.stdout == metrics_stdout.replace(
        f'{metrics_line}\n', '',
        1), (f'Metrics parity mismatch on {legacy_cmd}!\n'
             f'Without --metrics: {legacy.stdout}'
             f'With --metrics: {metrics_stdout}')

  def _RunImageCompareCommand(self, image_diff, image_matching_algorithm):
    algorithm = image_matching_algorithm
    extra_flags = []
    if isinstance(image_matching_algorithm, (tuple, list)):
      algorithm, extra_flags = image_matching_algorithm

    cmd = [self.pdfium_diff_path, '--metrics']
    if self.reverse_byte_order:
      cmd.append('--reverse-byte-order')
    if algorithm == FUZZY_MATCHING:
      cmd.extend(extra_flags)
    cmd.extend([image_diff.actual_path, image_diff.expected_path])

    # Failure to run the binary at all is fatal, and is left to propagate.
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)

    # `--metrics` reports the comparison result in-band, so a non-zero exit
    # means the comparison could not be performed at all.
    if result.returncode != 0:
      return subprocess.CalledProcessError(
          result.returncode, cmd, output=result.stdout, stderr=result.stderr)

    # Parse metrics line: <basename>: actual_w=...
    base_name = os.path.basename(image_diff.actual_path)
    metrics_line = None
    for line in result.stdout.splitlines():
      if line.startswith(f'{base_name}:'):
        metrics_line = line
        break

    if not metrics_line:
      return RuntimeError(
          f'Failed to find metric line for {base_name} in output: '
          f'{result.stdout}')

    tokens = metrics_line.split()
    kv = {}
    for token in tokens[1:]:
      if '=' in token:
        k, v = token.split('=', 1)
        kv[k] = v

    metrics = DiffMetrics(
        actual_w=int(kv['actual_w']),
        actual_h=int(kv['actual_h']),
        expected_w=int(kv['expected_w']),
        expected_h=int(kv['expected_h']),
        pixels=int(kv['pixels']),
        total=int(kv['total']),
        max_delta=int(kv['max_delta']),
        mse=float(kv['mse']),
        win_mse=float(kv['win_mse']),
        c_result=kv['c_result'])
    image_diff.metrics = metrics
    c_pass = (metrics.c_result == 'PASS')

    self._CheckMetricsParity(cmd, result.stdout, metrics_line, c_pass)

    # Evaluate decision in Python.
    python_pass, failure_reason = self.EvaluateMatch(metrics,
                                                     image_matching_algorithm)

    # SHADOW VALIDATION: Verify Python's decision matches C++'s decision!
    assert python_pass == c_pass, (
        f'Shadow check decision mismatch on {image_diff.actual_path} vs '
        f'{image_diff.expected_path}!\n'
        f'Python decision: {python_pass} (failure_reason: {failure_reason})\n'
        f'C++ decision: {c_pass} (c_result={metrics.c_result})\n'
        f'Metrics: {metrics}')

    if python_pass:
      return None

    return failure_reason

  def _RunImageDiffCommand(self, image_diff):
    # TODO(crbug.com/42270934): Diff mode ignores --reverse-byte-order.
    cmd = [
        self.pdfium_diff_path,
        '--subtract',
        image_diff.actual_path,
        image_diff.expected_path,
        image_diff.diff_path,
    ]
    # `--subtract` writes nothing when it considers the images the same, so
    # remove any diff left over from a previous run before checking below.
    if os.path.exists(image_diff.diff_path):
      os.unlink(image_diff.diff_path)
    subprocess.run(cmd, capture_output=True, check=False)
    return os.path.exists(image_diff.diff_path)

  def ComputeDifferences(self, input_filename, source_dir, working_dir,
                         image_matching_algorithm):
    """Computes differences between actual and expected image files.

    Returns:
      A list of `ImageDiff` instances, one per differing page.
    """
    image_diffs = []

    path_templates = _PathTemplates(input_filename, source_dir, working_dir,
                                    self.os_name, self.suffix_order)
    for page in itertools.count():
      page_diff = ImageDiff(actual_path=path_templates.GetActualPath(page))
      if not os.path.exists(page_diff.actual_path):
        expected_path = path_templates.GetExpectedPath(
            page, default_to_base=False)
        if expected_path:
          page_diff.expected_path = expected_path
          page_diff.reason = f'{page_diff.actual_path} does not exist'
          image_diffs.append(page_diff)
          continue
        # No more actual pages.
        break

      expected_path = path_templates.GetExpectedPath(page)
      if os.path.exists(expected_path):
        page_diff.expected_path = expected_path

        compare_error = self._RunImageCompareCommand(page_diff,
                                                     image_matching_algorithm)
        if compare_error:
          page_diff.reason = str(compare_error)
          if isinstance(compare_error, Exception):
            page_diff.comparison_failure = False
          else:
            # TODO(crbug.com/42270934): Compare and diff simultaneously.
            page_diff.diff_path = path_templates.GetDiffPath(page)
            if not self._RunImageDiffCommand(page_diff):
              print(f'WARNING: No diff for {page_diff.actual_path}')
              page_diff.diff_path = None
        else:
          # Validate that no other paths match exactly.
          for unexpected_path in path_templates.GetExpectedPaths(page)[1:]:
            page_diff.expected_path = unexpected_path
            if not self._RunImageCompareCommand(page_diff, EXACT_MATCHING):
              page_diff.reason = f'Also matches {unexpected_path}'
              break
          page_diff.expected_path = expected_path
      else:
        if page == 0:
          print(f'WARNING: no expected results files for {input_filename}')
        page_diff.reason = f'{expected_path} does not exist'

      if page_diff.reason:
        image_diffs.append(page_diff)

    return image_diffs

  def Regenerate(self, input_filename, source_dir, working_dir,
                 image_matching_algorithm):
    path_templates = _PathTemplates(input_filename, source_dir, working_dir,
                                    self.os_name, self.suffix_order)
    for page in itertools.count():
      expected_paths = path_templates.GetExpectedPaths(page)

      first_match = None
      last_match = None
      page_diff = ImageDiff(actual_path=path_templates.GetActualPath(page))
      if os.path.exists(page_diff.actual_path):
        # Match against all expected page images.
        for index, expected_path in enumerate(expected_paths):
          page_diff.expected_path = expected_path
          if not self._RunImageCompareCommand(page_diff,
                                              image_matching_algorithm):
            if first_match is None:
              first_match = index
            last_match = index

        if last_match == 0:
          # Regeneration not needed. This case may be reached if only some, but
          # not all, pages need to be regenerated.
          continue
      elif expected_paths:
        # Remove all expected page images.
        print(f'WARNING: {input_filename} has extra expected page {page}')
        first_match = 0
        last_match = len(expected_paths)
      else:
        # No more expected or actual pages.
        break

      # Try to reuse expectations by removing intervening non-matches.
      #
      # TODO(crbug.com/42270995): This can make mistakes due to a lack of
      # global knowledge about other test configurations, which is why it just
      # creates backup files rather than immediately removing files.
      if last_match is not None:
        if first_match > 1:
          print(f'WARNING: {input_filename}.{page} has non-adjacent match')
        if first_match != last_match:
          print(f'WARNING: {input_filename}.{page} has redundant matches')

        for expected_path in expected_paths[:last_match]:
          os.rename(expected_path, expected_path + '.bak')
        continue

      # Regenerate the most specific expected path that exists. If there are no
      # existing expectations, regenerate the base case.
      expected_path = path_templates.GetExpectedPath(page)
      shutil.copyfile(page_diff.actual_path, expected_path)
      self._RunCommand([_PNG_OPTIMIZER, expected_path])


_ACTUAL_TEMPLATE = '.pdf.%d.png'
_DIFF_TEMPLATE = '.pdf.%d.diff.png'


class _PathTemplates:

  def __init__(self, input_filename, source_dir, working_dir, os_name,
               suffix_order):
    input_root, _ = os.path.splitext(input_filename)
    self.actual_path_template = os.path.join(working_dir,
                                             input_root + _ACTUAL_TEMPLATE)
    self.diff_path_template = os.path.join(working_dir,
                                           input_root + _DIFF_TEMPLATE)

    # Pre-create the available templates from most to least specific. We
    # generally expect the most specific case to match first.
    self.expected_templates = []
    for suffix in suffix_order:
      formatted_suffix = suffix.format(os=os_name)
      self.expected_templates.append(
          os.path.join(
              source_dir,
              f'{input_root}_expected{formatted_suffix}{_ACTUAL_TEMPLATE}',
          ))
    assert self.expected_templates

  def GetActualPath(self, page):
    return self.actual_path_template % page

  def GetDiffPath(self, page):
    return self.diff_path_template % page

  def _GetPossibleExpectedPaths(self, page):
    return [template % page for template in self.expected_templates]

  def GetExpectedPaths(self, page):
    return list(filter(os.path.exists, self._GetPossibleExpectedPaths(page)))

  def GetExpectedPath(self, page, default_to_base=True):
    """Returns the most specific expected path that exists."""
    last_not_found_expected_path = None
    for expected_path in self._GetPossibleExpectedPaths(page):
      if os.path.exists(expected_path):
        return expected_path
      last_not_found_expected_path = expected_path
    return last_not_found_expected_path if default_to_base else None
