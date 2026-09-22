#!/usr/bin/env python3
# Copyright 2026 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Finds surprise passes (unexpected successes) in ResultDB for a Gerrit CL."""

import argparse
import collections
import json
import subprocess
import sys
import urllib.error
import urllib.request

from githelper import GitHelper


def GetLatestPatchset(issue):
  """Returns the highest patchset number for a Gerrit issue."""
  url = f'https://pdfium-review.googlesource.com/changes/{issue}'
  try:
    with urllib.request.urlopen(url, timeout=10) as response:
      raw = response.read().decode('utf-8')
    data = json.loads(raw.removeprefix(")]}'\n"))
    return data.get('current_revision_number')
  except (urllib.error.URLError, json.JSONDecodeError, ValueError, OSError):
    return None


def GetTryResults(issue, patchset=None):
  """Fetches tryjob results as JSON via git cl try-results."""
  if patchset:
    patchsets = [patchset]
  else:
    latest = GetLatestPatchset(issue)
    patchsets = range(latest, 0, -1) if latest else [None]

  for ps in patchsets:
    cmd = ['git', 'cl', 'try-results', '--json=-', '--issue', str(issue)]
    if ps is not None:
      cmd.extend(['--patchset', str(ps)])
    result = subprocess.run(cmd, capture_output=True, text=True, check=False)
    try:
      builds = json.loads(result.stdout)
      if builds:
        return builds
    except (json.JSONDecodeError, ValueError):
      pass
  return []


def QueryResultDB(invocation_ids, batch_size=50):
  """Queries ResultDB for test results across invocations."""
  for i in range(0, len(invocation_ids), batch_size):
    batch = invocation_ids[i:i + batch_size]
    cmd = ['rdb', 'query', '-json', '-tr-fields', 'testId,status,tags,variant'
          ] + batch
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    for line in proc.stdout:
      line = line.strip()
      if not line:
        continue
      try:
        data = json.loads(line)
        tr = data.get('testResult')
        if tr:
          yield tr
      except json.JSONDecodeError:
        pass
    proc.wait()


def ExtractTags(test_result):
  """Extracts tags list into a dictionary of key -> value."""
  tags = {}
  for item in test_result.get('tags', []):
    key = item.get('key')
    value = item.get('value')
    if key:
      tags[key] = value
  return tags


def main():
  parser = argparse.ArgumentParser(
      description='Query ResultDB for surprise passes in a Gerrit issue.')
  parser.add_argument(
      '--issue',
      '-i',
      help='Gerrit issue number (defaults to current branch issue)')
  parser.add_argument(
      '--patchset',
      '-p',
      help='Patchset number (defaults to latest patchset with tryjobs)')
  parser.add_argument(
      '--all-skips',
      action='store_true',
      help='Also include ordinary skipped tests in output')
  parser.add_argument(
      '--by-builder',
      action='store_true',
      help='Group results by builder instead of by test')
  parser.add_argument(
      '--json', action='store_true', help='Output raw results as JSON')
  args = parser.parse_args()

  git = GitHelper()
  branch = git.GetCurrentBranchName()
  issue = args.issue or git.GetGerritIssue(branch)
  if not issue:
    print(
        'Error: Could not determine Gerrit issue for current branch. '
        'Specify --issue manually.',
        file=sys.stderr)
    return 1

  if not args.json:
    print(f'Branch: {branch or "(detached HEAD)"}')
    print(f'Gerrit Issue: {issue}')

  tryjobs = GetTryResults(issue, patchset=args.patchset)
  if not tryjobs:
    if args.json:
      print('{}')
    else:
      print('No tryjob results found.')
    return 0

  # Filter out non-test builders that lack test invocations (e.g. presubmit).
  test_builds = [
      b for b in tryjobs
      if 'id' in b and b.get('builder', {}).get('builder') != 'pdfium_presubmit'
  ]
  invocation_ids = [f'build-{b["id"]}' for b in test_builds]

  if not args.json:
    print(f'Querying ResultDB across {len(invocation_ids)} tryjob builds...')

  surprises = []
  ordinary_skips = []

  for tr in QueryResultDB(invocation_ids):
    status = tr.get('status')
    if status != 'SKIP':
      continue

    tags = ExtractTags(tr)
    builder = tr.get('variant', {}).get('def', {}).get('builder', 'unknown')
    test_id = tr.get('testId', 'unknown')
    suppression_tag = tags.get('pdfium_suppression')

    entry = {
        'test_id': test_id,
        'builder': builder,
        'status': status,
        'pdfium_suppression': suppression_tag or '(none)',
        'is_surprise': suppression_tag == 'unexpected_success',
        'raw_status': tags.get('raw_status', '(none)'),
        'step_name': tags.get('step_name', '(none)')
    }

    if entry['is_surprise']:
      surprises.append(entry)
    else:
      ordinary_skips.append(entry)

  if args.json:
    out = {'surprises': surprises}
    if args.all_skips:
      out['ordinary_skips'] = ordinary_skips
    print(json.dumps(out, indent=2))
    return 0

  print('\n=== Surprise Passes (Unexpected Successes) ===')
  if not surprises:
    print('No surprise passes found.')
  elif args.by_builder:
    builder_map = collections.defaultdict(list)
    for s in surprises:
      builder_map[s['builder']].append(s)
    for builder in sorted(builder_map.keys()):
      entries = builder_map[builder]
      print(f'\nBuilder: {builder} ({len(entries)} entries)')
      for e in sorted(entries, key=lambda x: (x['test_id'], x['step_name'])):
        print(f'  {e["test_id"]}')
        print(f'    status: {e["status"]}')
        print(f'    tag:    pdfium_suppression={e["pdfium_suppression"]}')
        if e['step_name'] != '(none)':
          print(f'    step:   {e["step_name"]}')
  else:
    test_map = collections.defaultdict(list)
    for s in surprises:
      test_map[s['test_id']].append(s)
    for test_id in sorted(test_map.keys()):
      entries = test_map[test_id]
      tag_val = entries[0]['pdfium_suppression']
      status = entries[0]['status']
      print(f'\nTest: {test_id}')
      print(f'  status: {status}')
      print(f'  tag:    pdfium_suppression={tag_val}')
      steps = sorted(
          set(f'[{e["builder"]}] {e["step_name"]}' if e['step_name'] !=
              '(none)' else f'[{e["builder"]}]' for e in entries))
      print(f'  configurations ({len(entries)}):')
      for step in steps:
        print(f'    - {step}')

  print('\n----------------------------------------')
  distinct_tests = len(set(s['test_id'] for s in surprises))
  distinct_builders = len(set(s['builder'] for s in surprises))
  print(
      f'Total surprise passes: {len(surprises)} occurrences '
      f'across {distinct_tests} distinct tests on {distinct_builders} builders.'
  )

  if args.all_skips:
    print('\n=== Ordinary Skips (No Surprise Tag) ===')
    distinct_ord = len(set(s['test_id'] for s in ordinary_skips))
    print(f'Total ordinary skips: {len(ordinary_skips)} occurrences '
          f'across {distinct_ord} distinct tests.')

  return 0


if __name__ == '__main__':
  sys.exit(main())
