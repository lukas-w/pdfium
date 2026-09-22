#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Finds performance step functions (regressions or improvements) in CI."""

import argparse
import collections
import concurrent.futures
import datetime
import json
import os
import statistics
import subprocess
import sys
import tempfile


def ParseISO8601(date_str):
  """Parses an ISO-8601 date or date-time string into a UTC datetime."""
  norm_str = date_str[:-1] + '+00:00' if date_str.endswith('Z') else date_str
  dt = datetime.datetime.fromisoformat(norm_str)
  if dt.tzinfo is None:
    dt = dt.replace(tzinfo=datetime.timezone.utc)
  else:
    dt = dt.astimezone(datetime.timezone.utc)
  return dt


def GetCIBuilders():
  """Returns a list of all CI builders for the pdfium project."""
  cmd = ['bb', 'builders', 'pdfium/ci']
  proc = subprocess.run(cmd, capture_output=True, text=True, check=False)
  if proc.returncode != 0:
    err = proc.stderr.strip()
    if err:
      print(f'Warning: "bb builders" failed: {err}', file=sys.stderr)
    return []
  builders = []
  for line in proc.stdout.splitlines():
    line = line.strip()
    if line.startswith('pdfium/ci/'):
      builders.append(line.replace('pdfium/ci/', ''))
  return sorted(builders)


def GetBuildsForBuilder(builder, since_dt, until_dt):
  """Fetches successful builds for `builder` within the date range."""
  predicate = json.dumps({
      'builder': {
          'project': 'pdfium',
          'bucket': 'ci',
          'builder': builder,
      },
      'status': 'SUCCESS',
      'createTime': {
          'startTime': since_dt.strftime('%Y-%m-%dT%H:%M:%S.%fZ'),
          'endTime': until_dt.strftime('%Y-%m-%dT%H:%M:%S.%fZ'),
      },
  })
  cmd = ['bb', 'ls', '-predicate', predicate, '-json']
  builds = []
  proc = subprocess.Popen(
      cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
  with proc:
    for line in proc.stdout:
      line = line.strip()
      if not line:
        continue
      try:
        d = json.loads(line)
        create_time = d.get('createTime') or d.get('startTime')
        if not create_time:
          continue
        ct = ParseISO8601(create_time)
        if ct > until_dt or ct < since_dt:
          continue

        build_id = d.get('id')
        build_num = d.get('number')
        commit = (d.get('input') or {}).get('gitilesCommit', {}).get('id', '')
        if not commit:
          for t in d.get('tags') or []:
            val = t.get('value') or ''
            if t.get('key') == 'buildset' and 'commit/gitiles' in val:
              commit = val.split('/')[-1]
              break
        if build_id:
          builds.append({
              'id': build_id,
              'number': build_num,
              'commit': commit,
              'create_time': create_time,
          })
      except (json.JSONDecodeError, ValueError):
        pass
    proc.wait()
    if proc.returncode != 0:
      err = proc.stderr.read().strip()
      if err:
        print(f'Warning: "bb ls" failed for {builder}: {err}', file=sys.stderr)
  # Buildbucket returns newest first; reverse so order is chronological.
  builds.reverse()
  return builds


def QueryResultDBForBuilds(build_ids, batch_size=25):
  """Queries ResultDB in batches and yields (build_id, test_result) tuples."""
  for i in range(0, len(build_ids), batch_size):
    batch = build_ids[i:i + batch_size]
    inv_args = [f'build-{bid}' for bid in batch]
    cmd = ['rdb', 'query', '-json', '-tr-fields', 'testId,duration,variant'
          ] + inv_args
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    with proc:
      for line in proc.stdout:
        line = line.strip()
        if not line:
          continue
        try:
          data = json.loads(line)
          tr = data.get('testResult')
          inv_id = (data.get('invocationId') or '').replace('build-', '')
          if tr and inv_id:
            yield inv_id, tr
        except json.JSONDecodeError:
          pass
      proc.wait()
      if proc.returncode != 0:
        err = proc.stderr.read().strip()
        if err:
          print(f'Warning: "rdb query" failed: {err}', file=sys.stderr)


SUITE_ALIASES = {
    'unittest': 'pdfium_unittests',
    'unittests': 'pdfium_unittests',
    'embedder': 'pdfium_embeddertests',
    'embeddertest': 'pdfium_embeddertests',
    'embeddertests': 'pdfium_embeddertests',
}


def MatchesSuiteFilter(suite, suite_filters, exact=False):
  """Returns True if `suite` matches any filter in `suite_filters`."""
  if not suite_filters:
    return True
  suite_lower = suite.lower()
  if exact:
    return suite_lower in (
        SUITE_ALIASES.get(sf.lower(), sf.lower()) for sf in suite_filters)
  # Strip '_javascript_disabled' when matching general filters like 'javascript'
  # so that 'corpus_javascript_disabled' and 'pixel_javascript_disabled' are not
  # mistakenly matched by '--javascript'.
  base_suite = suite_lower.replace('_javascript_disabled', '')
  for sf in suite_filters:
    if 'javascript_disabled' in sf:
      if sf in suite_lower:
        return True
    elif sf in base_suite:
      return True
  return False


def ExtractTestData(build_ids,
                    suite_filters=None,
                    exact_suite=False,
                    use_cache=True,
                    verbose=False):
  """Collects suite sums, test counts, and test durations across `build_ids`."""
  test_timings = collections.defaultdict(lambda: collections.defaultdict(dict))
  suite_totals = collections.defaultdict(lambda: collections.defaultdict(float))
  suite_counts = collections.defaultdict(lambda: collections.defaultdict(int))

  cache_dir = os.path.join(tempfile.gettempdir(), 'pdfium_rdb_cache')
  if use_cache:
    os.makedirs(cache_dir, exist_ok=True)

  needed_build_ids = []
  for bid in build_ids:
    cache_path = os.path.join(cache_dir, f'{bid}.json') if use_cache else None
    if cache_path and os.path.isfile(cache_path):
      try:
        with open(cache_path, 'r', encoding='utf-8') as f:
          build_data = json.load(f)
        for suite, tests in build_data.items():
          if suite_filters and not MatchesSuiteFilter(
              suite, suite_filters, exact=exact_suite):
            continue
          for test_id, dur in tests.items():
            test_timings[bid][suite][test_id] = dur
            suite_totals[bid][suite] += dur
            suite_counts[bid][suite] += 1
        continue
      except (json.JSONDecodeError, OSError):
        pass
    needed_build_ids.append(bid)

  if needed_build_ids:
    if verbose:
      print(
          f'Querying ResultDB for {len(needed_build_ids)} builds...',
          file=sys.stderr,
          flush=True)

    new_raw = collections.defaultdict(
        lambda: collections.defaultdict(lambda: collections.defaultdict(list)))
    for bid, tr in QueryResultDBForBuilds(needed_build_ids):
      variant = (tr.get('variant') or {}).get('def') or {}
      suite = variant.get('test_suite') or 'unknown'
      test_id = tr.get('testId') or 'unknown'
      dur_str = tr.get('duration') or '0s'
      try:
        dur = float(dur_str.rstrip('s')) if dur_str.endswith('s') else 0.0
      except ValueError:
        dur = 0.0

      new_raw[bid][suite][test_id].append(dur)

    for bid in needed_build_ids:
      suites = new_raw[bid]
      build_cache_data = {}
      for suite, tests in suites.items():
        build_cache_data[suite] = {}
        include_suite = not suite_filters or MatchesSuiteFilter(
            suite, suite_filters, exact=exact_suite)
        for test_id, durs in tests.items():
          mean_dur = durs[0] if len(durs) == 1 else (sum(durs) / len(durs))
          build_cache_data[suite][test_id] = mean_dur
          if include_suite:
            test_timings[bid][suite][test_id] = mean_dur
            suite_totals[bid][suite] += mean_dur
            suite_counts[bid][suite] += 1

      if use_cache and build_cache_data:
        cache_path = os.path.join(cache_dir, f'{bid}.json')
        try:
          with open(cache_path, 'w', encoding='utf-8') as f:
            json.dump(build_cache_data, f)
        except OSError:
          pass

  return suite_totals, suite_counts, test_timings


def DetectSuiteSteps(runs,
                     suite_totals,
                     suite_counts=None,
                     threshold_pct=15.0,
                     min_delta=2.0,
                     persistence_runs=2,
                     suite_filters=None,
                     exact_suite=False):
  """Detects persistent step functions in suite durations.

  A step occurs at index `i` if the median of `runs[i : i + 1 +
  persistence_runs]` differs from the pre-change baseline (median of
  `runs[i - 3 : i]`) by at least `threshold_pct` and `min_delta` seconds,
  and every run in the post window maintains that shift.
  """
  all_suites = sorted(
      set().union(*(suite_totals[r['id']].keys() for r in runs)))
  if suite_filters:
    all_suites = [
        s for s in all_suites
        if MatchesSuiteFilter(s, suite_filters, exact=exact_suite)
    ]
  post_window_size = persistence_runs + 1
  step_events = []

  for suite in all_suites:
    # Filter to runs where this suite had test results.
    suite_runs = [r for r in runs if suite in suite_totals[r['id']]]
    if len(suite_runs) < 3 + post_window_size:
      continue

    durations = [suite_totals[r['id']][suite] for r in suite_runs]
    counts = [
        suite_counts[r['id']][suite] if suite_counts else 0 for r in suite_runs
    ]
    idx = 3
    limit = len(suite_runs) - post_window_size

    while idx <= limit:
      pre_window = durations[idx - 3:idx]
      post_window = durations[idx:idx + post_window_size]

      m_pre = statistics.median(pre_window)
      m_post = statistics.median(post_window)

      delta = m_post - m_pre
      pct = (delta / m_pre * 100.0) if m_pre > 0.0 else 0.0

      if abs(pct) >= threshold_pct and abs(delta) >= min_delta:
        c_pre = statistics.median(counts[idx - 3:idx])
        c_post = statistics.median(counts[idx:idx + post_window_size])
        if delta > 0:
          # Regression: all post-runs must remain elevated.
          min_allowed = m_pre * (1.0 + (threshold_pct / 200.0))
          if all(d >= min_allowed for d in post_window):
            step_events.append({
                'type': 'REGRESSION',
                'suite': suite,
                'run_index': idx,
                'pre_runs': suite_runs[idx - 3:idx],
                'trigger_run': suite_runs[idx],
                'prev_run': suite_runs[idx - 1],
                'future_runs': suite_runs[idx + 1:idx + post_window_size],
                'm_pre': m_pre,
                'm_post': m_post,
                'delta': delta,
                'pct': pct,
                'count_pre': int(c_pre),
                'count_post': int(c_post),
            })
            idx += post_window_size
            continue
        else:
          # Improvement: all post-runs must remain lower.
          max_allowed = m_pre * (1.0 - (threshold_pct / 200.0))
          if all(d <= max_allowed for d in post_window):
            step_events.append({
                'type': 'IMPROVEMENT',
                'suite': suite,
                'run_index': idx,
                'pre_runs': suite_runs[idx - 3:idx],
                'trigger_run': suite_runs[idx],
                'prev_run': suite_runs[idx - 1],
                'future_runs': suite_runs[idx + 1:idx + post_window_size],
                'm_pre': m_pre,
                'm_post': m_post,
                'delta': delta,
                'pct': pct,
                'count_pre': int(c_pre),
                'count_post': int(c_post),
            })
            idx += post_window_size
            continue
      idx += 1

  return step_events


def FindCulpritTests(step_event,
                     test_timings,
                     top_n=5,
                     test_threshold_pct=30.0,
                     test_min_delta=0.2):
  """Finds individual tests that persistently shifted across the step."""
  suite = step_event['suite']
  pre_runs = step_event.get('pre_runs', [step_event['prev_run']])
  pre_ids = [r['id'] for r in pre_runs]
  trigger_id = step_event['trigger_run']['id']
  future_ids = [r['id'] for r in step_event['future_runs']]
  post_ids = [trigger_id] + future_ids

  all_test_ids = set()
  for bid in pre_ids + post_ids:
    all_test_ids.update(test_timings[bid].get(suite, {}).keys())

  candidates = []
  is_regression = step_event['type'] == 'REGRESSION'

  for test_id in all_test_ids:
    pre_durs = [
        test_timings[bid][suite][test_id]
        for bid in pre_ids
        if test_id in test_timings[bid].get(suite, {})
    ]
    post_durs = [
        test_timings[bid][suite][test_id]
        for bid in post_ids
        if test_id in test_timings[bid].get(suite, {})
    ]

    # Test must run at least once in each epoch to compare durations.
    if not pre_durs or not post_durs:
      continue

    t_pre = statistics.median(pre_durs)
    m_test_post = statistics.median(post_durs)
    t_delta = m_test_post - t_pre
    t_pct = (t_delta / t_pre * 100.0) if t_pre > 0.0 else 0.0

    if abs(t_pct) >= test_threshold_pct and abs(t_delta) >= test_min_delta:
      if is_regression and t_delta > 0 and all(d >= t_pre for d in post_durs):
        candidates.append({
            'test_id': test_id,
            'pre_dur': t_pre,
            'post_durs': post_durs,
            'm_post': m_test_post,
            'delta': t_delta,
            'pct': t_pct,
        })
      elif (not is_regression and t_delta < 0 and
            all(d <= t_pre for d in post_durs)):
        candidates.append({
            'test_id': test_id,
            'pre_dur': t_pre,
            'post_durs': post_durs,
            'm_post': m_test_post,
            'delta': t_delta,
            'pct': t_pct,
        })

  # Rank by absolute delta descending.
  candidates.sort(key=lambda x: abs(x['delta']), reverse=True)
  total_candidate_delta = sum(c['delta'] for c in candidates)
  return {
      'candidates': candidates[:top_n],
      'total_count': len(candidates),
      'total_delta': total_candidate_delta,
  }


def AnalyzeBuilder(builder,
                   since_dt,
                   until_dt,
                   threshold_pct,
                   min_delta,
                   persistence_runs,
                   top_n_culprits,
                   test_threshold_pct=30.0,
                   test_min_delta=0.2,
                   suite_filters=None,
                   exact_suite=False,
                   use_cache=True,
                   verbose=False):
  """Fetches and evaluates performance step functions for `builder`."""
  if verbose:
    print(
        f'Fetching builds for {builder} between {since_dt.isoformat()} '
        f'and {until_dt.isoformat()}...',
        file=sys.stderr,
        flush=True)

  builds = GetBuildsForBuilder(builder, since_dt, until_dt)
  if len(builds) < 3 + 1 + persistence_runs:
    if verbose:
      print(
          f'Insufficient builds found for {builder} ({len(builds)} builds).',
          file=sys.stderr)
    return []

  build_ids = [b['id'] for b in builds]
  suite_totals, suite_counts, test_timings = ExtractTestData(
      build_ids,
      suite_filters=suite_filters,
      exact_suite=exact_suite,
      use_cache=use_cache,
      verbose=verbose)

  step_events = DetectSuiteSteps(
      runs=builds,
      suite_totals=suite_totals,
      suite_counts=suite_counts,
      threshold_pct=threshold_pct,
      min_delta=min_delta,
      persistence_runs=persistence_runs,
      suite_filters=suite_filters,
      exact_suite=exact_suite)

  results = []
  for step in step_events:
    culprit_info = FindCulpritTests(
        step_event=step,
        test_timings=test_timings,
        top_n=top_n_culprits,
        test_threshold_pct=test_threshold_pct,
        test_min_delta=test_min_delta)
    results.append({
        'builder': builder,
        'step': step,
        'culprits': culprit_info['candidates'],
        'culprit_total_count': culprit_info['total_count'],
        'culprit_total_delta': culprit_info['total_delta'],
    })

  return results


def GetCommitRangeUrl(prev_commit, trigger_commit):
  """Returns a Gitiles log URL between `prev_commit` and `trigger_commit`."""
  if prev_commit and trigger_commit and prev_commit != trigger_commit:
    return (f'https://pdfium.googlesource.com/pdfium/+log/'
            f'{prev_commit[:10]}..{trigger_commit[:10]}')
  return ''


def FormatEventText(event):
  """Formats a single step function event for text display."""
  builder = event['builder']
  step = event['step']
  culprits = event['culprits']
  culprit_total_count = event.get('culprit_total_count', len(culprits))
  culprit_total_delta = event.get('culprit_total_delta',
                                  sum(c['delta'] for c in culprits))
  trigger = step['trigger_run']
  prev = step['prev_run']
  future = step['future_runs']

  stype = step['type']
  sign = '+' if step['delta'] > 0 else ''
  trigger_commit = trigger.get('commit') or ''
  prev_commit = prev.get('commit') or ''
  range_url = GetCommitRangeUrl(prev_commit, trigger_commit)
  range_line = f'  Commit Range:  {range_url}\n' if range_url else ''

  test_count_line = ''
  c_pre = step.get('count_pre', 0)
  c_post = step.get('count_post', 0)
  if c_pre or c_post:
    c_delta = c_post - c_pre
    csign = '+' if c_delta > 0 else ''
    test_count_line = f'  Tests Run:     {c_pre} -> {c_post} ({csign}{c_delta} tests)\n'

  header = (f'=== [{stype}] builder: {builder} | suite: {step["suite"]} ===\n'
            f'  Trigger Build: #{trigger.get("number") or "unknown"} '
            f'(commit: {(trigger_commit or "unknown")[:10]})\n'
            f'  Pre Build:     #{prev.get("number") or "unknown"} '
            f'(commit: {(prev_commit or "unknown")[:10]})\n'
            f'{range_line}'
            f'  Date:          {trigger.get("create_time") or "unknown"}\n'
            f'{test_count_line}'
            f'  Suite Shift:   {step["m_pre"]:.2f}s -> {step["m_post"]:.2f}s '
            f'({sign}{step["pct"]:.1f}%, {sign}{step["delta"]:.2f}s)\n'
            f'  Persisted in:  ' +
            ', '.join(f'#{r.get("number") or "?"}' for r in future))

  lines = [header]
  if culprits:
    csign = '+' if culprit_total_delta > 0 else ''
    lines.append(
        f'  Culprit Tests (showing top {len(culprits)} of {culprit_total_count}, '
        f'accounting for {csign}{culprit_total_delta:.2f}s of {sign}{step["delta"]:.2f}s shift):'
    )
    for c in culprits:
      c_item_sign = '+' if c['delta'] > 0 else ''
      post_str = ', '.join(f'{d:.2f}s' for d in c['post_durs'])
      lines.append(
          f'    - {c["test_id"]}\n'
          f'        duration: {c["pre_dur"]:.2f}s -> ({post_str}) '
          f'[{c_item_sign}{c["pct"]:.1f}%, {c_item_sign}{c["delta"]:.2f}s]')
  else:
    lines.append(
        '  Culprit Tests: None met threshold (diffuse across entire suite)')

  return '\n'.join(lines)


def main():
  parser = argparse.ArgumentParser(
      description='Find performance step functions across CI builders.')
  parser.add_argument(
      '--since',
      help='Start date/time in ISO-8601 format (e.g. "2026-09-14" or '
      '"2026-09-14T00:00:00Z"). Defaults to 7 days ago.')
  parser.add_argument(
      '--until',
      help='End date/time in ISO-8601 format (e.g. "2026-09-21" or '
      '"2026-09-21T23:59:59Z"). Defaults to current time.')
  parser.add_argument(
      '--builder',
      '-b',
      help='Specific CI builder to inspect (e.g. "linux", or comma-separated '
      'list). Defaults to "linux" if omitted and --all-builders is not set.')
  parser.add_argument(
      '--all-builders',
      action='store_true',
      help='Inspect all active CI builders in pdfium/ci.')
  parser.add_argument(
      '--threshold',
      type=float,
      default=15.0,
      help='Relative threshold percentage to flag a step function '
      '(default: 15.0)')
  parser.add_argument(
      '--min-delta',
      type=float,
      default=2.0,
      help='Minimum absolute change in seconds to flag a step function '
      '(default: 2.0)')
  parser.add_argument(
      '--persistence',
      type=int,
      default=2,
      help='Number of future runs required to confirm persistence (default: 2)')
  parser.add_argument(
      '--top-culprits',
      type=int,
      default=5,
      help='Number of individual culprit tests to report per step (default: 5)')
  parser.add_argument(
      '--test-threshold',
      type=float,
      default=30.0,
      help='Relative percentage threshold for individual culprit tests '
      '(default: 30.0)')
  parser.add_argument(
      '--test-min-delta',
      type=float,
      default=0.2,
      help='Minimum absolute change in seconds for individual culprit tests '
      '(default: 0.2)')
  parser.add_argument(
      '--suite',
      '-s',
      help='Filter to specific test suite(s), comma-separated or substring '
      '(e.g. "unittests", "embeddertests", "javascript", "corpus", "pixel")')
  parser.add_argument(
      '--exact-suite',
      action='store_true',
      help='Require exact suite name match instead of substring matching')
  parser.add_argument(
      '--unittest',
      '--unittests',
      action='store_true',
      help='Shortcut to filter for unit tests')
  parser.add_argument(
      '--corpus',
      action='store_true',
      help='Shortcut to filter for corpus tests')
  parser.add_argument(
      '--pixel', action='store_true', help='Shortcut to filter for pixel tests')
  parser.add_argument(
      '--embeddertest',
      '--embeddertests',
      action='store_true',
      help='Shortcut to filter for embedder tests')
  parser.add_argument(
      '--javascript',
      action='store_true',
      help='Shortcut to filter for javascript tests')
  parser.add_argument(
      '--concurrency',
      '-j',
      type=int,
      default=4,
      help='Number of concurrent builders to process in parallel (default: 4)')
  parser.add_argument(
      '--no-cache',
      action='store_true',
      help='Disable local disk caching of ResultDB build queries')
  parser.add_argument(
      '--json', action='store_true', help='Output results as JSON')
  parser.add_argument(
      '--verbose',
      '-v',
      action='store_true',
      help='Print verbose progress messages')

  args = parser.parse_args()

  if args.persistence < 1:
    print('Error: --persistence must be at least 1.', file=sys.stderr)
    return 1
  if args.threshold <= 0.0:
    print('Error: --threshold must be positive.', file=sys.stderr)
    return 1
  if args.min_delta < 0.0:
    print('Error: --min-delta must be non-negative.', file=sys.stderr)
    return 1
  if args.test_threshold <= 0.0:
    print('Error: --test-threshold must be positive.', file=sys.stderr)
    return 1
  if args.test_min_delta < 0.0:
    print('Error: --test-min-delta must be non-negative.', file=sys.stderr)
    return 1

  now = datetime.datetime.now(datetime.timezone.utc)
  if args.until:
    try:
      until_dt = ParseISO8601(args.until)
      if len(args.until) <= 10:
        until_dt = until_dt.replace(
            hour=23, minute=59, second=59, microsecond=999999)
    except ValueError as e:
      print(f'Error: Invalid ISO-8601 format for --until: {e}', file=sys.stderr)
      return 1
  else:
    until_dt = now

  if args.since:
    try:
      since_dt = ParseISO8601(args.since)
    except ValueError as e:
      print(f'Error: Invalid ISO-8601 format for --since: {e}', file=sys.stderr)
      return 1
  else:
    since_dt = now - datetime.timedelta(days=7)

  if since_dt >= until_dt:
    print(
        'Error: --since date must be earlier than --until date.',
        file=sys.stderr)
    return 1

  suite_filters = []
  if args.suite:
    suite_filters.extend(
        [s.strip().lower() for s in args.suite.split(',') if s.strip()])
  if args.unittest:
    suite_filters.append('unittests')
  if args.corpus:
    suite_filters.append('corpus')
  if args.pixel:
    suite_filters.append('pixel')
  if args.embeddertest:
    suite_filters.append('embeddertests')
  if args.javascript:
    suite_filters.append('javascript')

  if args.all_builders:
    builders = GetCIBuilders()
  elif args.builder:
    builders = [b.strip() for b in args.builder.split(',') if b.strip()]
  else:
    builders = ['linux']

  if not builders:
    print('Error: No builders specified or found.', file=sys.stderr)
    return 1

  if not args.json:
    print(f'Analyzing builds from {since_dt.isoformat()} to '
          f'{until_dt.isoformat()} for builders: {", ".join(builders)}')
    if suite_filters:
      print(f'Suite filters: {", ".join(suite_filters)}')
    print(f'Threshold: {args.threshold}%, Min Delta: {args.min_delta}s, '
          f'Persistence: {args.persistence} future runs\n')

  all_events = []
  if len(builders) > 1 and args.concurrency > 1:
    with concurrent.futures.ThreadPoolExecutor(
        max_workers=min(args.concurrency, len(builders))) as executor:
      futures = {
          executor.submit(
              AnalyzeBuilder,
              builder=b,
              since_dt=since_dt,
              until_dt=until_dt,
              threshold_pct=args.threshold,
              min_delta=args.min_delta,
              persistence_runs=args.persistence,
              top_n_culprits=args.top_culprits,
              test_threshold_pct=args.test_threshold,
              test_min_delta=args.test_min_delta,
              suite_filters=suite_filters,
              exact_suite=args.exact_suite,
              use_cache=not args.no_cache,
              verbose=args.verbose):
              b for b in builders
      }
      for future in concurrent.futures.as_completed(futures):
        b = futures[future]
        try:
          events = future.result()
          all_events.extend(events)
        except Exception as e:
          print(f'Error analyzing builder {b}: {e}', file=sys.stderr)
  else:
    for b in builders:
      try:
        events = AnalyzeBuilder(
            builder=b,
            since_dt=since_dt,
            until_dt=until_dt,
            threshold_pct=args.threshold,
            min_delta=args.min_delta,
            persistence_runs=args.persistence,
            top_n_culprits=args.top_culprits,
            test_threshold_pct=args.test_threshold,
            test_min_delta=args.test_min_delta,
            suite_filters=suite_filters,
            exact_suite=args.exact_suite,
            use_cache=not args.no_cache,
            verbose=args.verbose)
        all_events.extend(events)
      except Exception as e:
        print(f'Error analyzing builder {b}: {e}', file=sys.stderr)

  # Sort events deterministically by builder and trigger time.
  all_events.sort(key=lambda e: (e['builder'], e['step']['trigger_run'].get(
      'create_time') or ''))

  if args.json:
    json_output = []
    for event in all_events:
      step = event['step']
      c_pre = step.get('count_pre')
      c_post = step.get('count_post')
      c_delta = (c_post -
                 c_pre) if (c_pre is not None and c_post is not None) else None
      json_output.append({
          'builder':
              event['builder'],
          'type':
              step['type'],
          'suite':
              step['suite'],
          'trigger_build_number':
              step['trigger_run'].get('number'),
          'trigger_build_id':
              step['trigger_run'].get('id'),
          'trigger_commit':
              step['trigger_run'].get('commit'),
          'trigger_time':
              step['trigger_run'].get('create_time'),
          'pre_build_number':
              step['prev_run'].get('number'),
          'pre_commit':
              step['prev_run'].get('commit'),
          'commit_range_url':
              GetCommitRangeUrl(step['prev_run'].get('commit') or '',
                                step['trigger_run'].get('commit') or ''),
          'test_count_pre':
              c_pre,
          'test_count_post':
              c_post,
          'test_count_delta':
              c_delta,
          'median_pre_seconds':
              step['m_pre'],
          'median_post_seconds':
              step['m_post'],
          'delta_seconds':
              step['delta'],
          'percent_change':
              step['pct'],
          'future_build_numbers': [
              r.get('number') for r in step['future_runs']
          ],
          'culprits':
              event['culprits'],
          'culprit_total_count':
              event.get('culprit_total_count'),
          'culprit_total_delta':
              event.get('culprit_total_delta'),
      })
    print(json.dumps(json_output, indent=2))
    return 0

  if not all_events:
    print('No persistent performance step functions detected.')
    return 0

  for event in all_events:
    print(FormatEventText(event))
    print()

  print(f'Total step events found: {len(all_events)}')
  return 0


if __name__ == '__main__':
  sys.exit(main())
