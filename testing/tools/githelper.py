# Copyright 2017 The PDFium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Classes for dealing with git."""

from common import RunCommandPropagateErr


class GitHelper:
  """Issues git commands. Stateful."""

  def __init__(self):
    self.stashed = 0

  def Checkout(self, branch):
    """Checks out a branch."""
    RunCommandPropagateErr(['git', 'checkout', branch], exit_status_on_error=1)

  def FetchOriginMaster(self):
    """Fetches new changes on origin/main."""
    RunCommandPropagateErr(['git', 'fetch', 'origin', 'main'],
                           exit_status_on_error=1)

  def StashPush(self):
    """Stashes uncommitted changes."""
    output = RunCommandPropagateErr(['git', 'stash', '--include-untracked'],
                                    exit_status_on_error=1)
    if 'No local changes to save' in output:
      return False

    self.stashed += 1
    return True

  def StashPopAll(self):
    """Pops as many changes as this instance stashed."""
    while self.stashed > 0:
      RunCommandPropagateErr(['git', 'stash', 'pop'], exit_status_on_error=1)
      self.stashed -= 1

  def GetCurrentBranchName(self):
    """Returns a string with the current branch name."""
    return RunCommandPropagateErr(['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
                                  exit_status_on_error=1).strip()

  def GetCurrentBranchHash(self):
    return RunCommandPropagateErr(['git', 'rev-parse', 'HEAD'],
                                  exit_status_on_error=1).strip()

  def GetGerritIssue(self, branch=None):
    """Returns the Gerrit issue number for the branch or current issue."""
    if branch:
      issue = RunCommandPropagateErr(
          ['git', 'config', '--default', '', f'branch.{branch}.gerritissue'])
      if issue and issue.strip():
        return issue.strip()

    output = RunCommandPropagateErr(['git', 'cl', 'issue'])
    if output and 'Issue number:' in output:
      issue = output.split('Issue number:')[1].split('(')[0].strip()
      if issue and issue != 'None':
        return issue
    return None

  def IsCurrentBranchClean(self):
    output = RunCommandPropagateErr(['git', 'status', '--porcelain'],
                                    exit_status_on_error=1)
    return not output

  def BranchExists(self, branch_name):
    """Return whether a branch with the given name exists."""
    output = RunCommandPropagateErr(
        ['git', 'rev-parse', '--verify', branch_name])
    return output is not None

  def CloneLocal(self, source_repo, new_repo):
    RunCommandPropagateErr(['git', 'clone', source_repo, new_repo],
                           exit_status_on_error=1)
