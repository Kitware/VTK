#!/usr/bin/env python3

"""
backport-commits-for-milestone

Create a branch to backport merged MRs that are tagged with a milestone, but
are not present in the corresponding release branch.

For each of these MRs, check if it has been merged into VTK's release branch.
For each that has not, attempt to bring it into the release branch, merging the
original code if possible, or cherry-picking those that are not. Note that
cherry-pick'ing branches that contain merges or end up conflicting cause the
entire MR in question to be skipped.

At the end, a report of the MRs found and acted on are listed.

This script will create a new branch named "backport-missed-mrs-{milestone}"
and assumes that the main VTK repository is using the "origin" remote.
"""

import argparse
import git
import gitlabapi
import json
import os
import shutil
import sys

try:
    from progress.bar import Bar
except ImportError:
    class Bar(object):
        def __init__(self, *args, **kwargs):
            pass

        def next(self):
            pass

        def iter(self, i):
            return i

        def finish(self):
            pass

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument('-t', '--token',
                        help='gitlab api token (gitlab->settings->access tokens)',
                        required=True)

arg_parser.add_argument('-m', '--milestone',
                        help='milestone to query for merge requests',
                        required=True)

args = arg_parser.parse_args()

# Access the repository containing this script
main_repo = git.Repo('.')

# Update the main repository.
main_repo.git.fetch('origin')

# Gather the list of merged MRs.
MERGE_REQUEST_TRAILER_PREFIX = 'Merge-request: !'
merged_mr_ids = set()
branch_point = None

bar = Bar('Searching for merged merge requests... %(hexsha)s')
bar.hexsha = '0000000000000000000000000000000000000000'
parents = main_repo.git.rev_list('--first-parent', '--min-parents=2', 'origin/release')

for sha in bar.iter(parents.split('\n')):
    sha = sha.rstrip()
    if not sha:
        continue
    commit = main_repo.commit(sha)
    bar.hexsha = commit.hexsha
    # See if we're still tracking merges into the release branch.
    if not commit.summary.endswith('into release') and branch_point is None:
        branch_point = commit.hexsha
    for line in commit.message.split('\n'):
        if line.startswith(MERGE_REQUEST_TRAILER_PREFIX):
            mr_id = int(line[len(MERGE_REQUEST_TRAILER_PREFIX):])
            merged_mr_ids.add(mr_id)

bar = Bar('Searching for the first ineligible commit... %(hexsha)s')
bar.hexsha = '0000000000000000000000000000000000000000'
parents = main_repo.git.rev_list('--first-parent', 'origin/master')
master_merge_commits = set()

# Find the first ineligible commit in the release branch.
ineligible_commit = None
for sha in bar.iter(parents.split('\n')):
    sha = sha.rstrip()
    if not sha:
        continue
    if sha == branch_point:
        break
    bar.hexsha = sha
    master_merge_commits.add(sha)
    ineligible_commit = sha

# Access gitlab using a token
gl = gitlabapi.Gitlab('gitlab.kitware.com', token=args.token)

# Grab the merged mrs with the milestone of interest.
merged_requests = gl.getmergerequests('vtk%2fvtk',
    milestone=args.milestone,
    sort='asc',
    state='merged')

branch_name = f'backport-missed-mrs-{args.milestone}'
try:
    shutil.rmtree('workdir')
except:
    pass
main_repo.git.worktree('add', '-f', 'workdir', 'origin/release')
repo = git.Repo('workdir')

# Create a branch based off of release for performing the backports
working_branch = repo.create_head(branch_name, commit='origin/release', force=True)
working_branch.checkout()

manual_mrs = []
merged_mrs = []
skipped_mrs = []

bar = Bar('Scanning merge requests... !%(iid)d')
bar.iid = 0

for mr in bar.iter(merged_requests):
    bar.iid = mr['iid']

    # This MR has already landed.
    if mr['iid'] in merged_mr_ids:
        continue

    # If this MR can be merged, use it as-is.
    needs_cherry_pick = repo.is_ancestor(ineligible_commit, mr['sha'])
    manual = None

    if needs_cherry_pick:
        # Create a branch to cherry-pick onto.
        backport_head = repo.create_head(f'backport-mr-{mr["iid"]}-for-{args.milestone}', 'origin/release', force=True)
        backport_head.checkout(force=True)

        commits = []
        for commit in repo.iter_commits(f'origin/release..{mr["sha"]}'):
            # We found this MR's branch point from master. Stop collecting commits from the branch.
            # Note that we are assuming merge requests to not merge master back
            # into themselves to resolve conflicts; we are blind before that
            # point.
            if commit.hexsha in master_merge_commits:
                break
            commits.append(commit)

        commits.reverse()
        for commit in commits:
            if not commit.parents:
                manual = 'root commit found'
                break
            if len(commit.parents) > 1:
                manual = 'merge commit found'
                break
            try:
                repo.git.cherry_pick(commit.hexsha, x=True)
            except git.GitCommandError as e:
                errstr = str(e)
                if 'git commit --allow-empty' in errstr:
                    repo.git.cherry_pick('--abort')
                elif 'CONFLICT' in errstr:
                    manual = f'cherry-pick failed (conflict)'
                    break
                else:
                    manual = f'cherry-pick failed ({e})'
                    break

        working_branch.checkout(force=True)
        if manual:
            repo.delete_head(backport_head, force=True)
    else:
        backport_head = repo.create_head(f'{mr["source_branch"]}-for-{args.milestone}', mr['sha'], force=True)

    if manual is not None:
        manual_mrs.append((mr, manual))
    else:
        merge_base = repo.merge_base(backport_head, working_branch)
        try:
            repo.index.merge_tree(backport_head, base=merge_base)
            new_commit = repo.index.commit(
                f'''Merge topic '{mr["source_branch"]}' into {branch_name}

Merge-request: !{mr["iid"]}
''',
                parent_commits=(working_branch.commit, backport_head.commit))

            old_parent = repo.commit(f'{branch_name}~')
            diff_index = repo.index.diff(old_parent)
            if len(diff_index) == 0:
                repo.git.reset(old_parent.hexsha)
                skipped_mrs.append(mr)
            else:
                merged_mrs.append(mr)
        except git.GitCommandError as e:
            manual_mrs.append((mr, e))

if skipped_mrs:
    print('Merge requests which appear to have already been backported another way:')
    for mr in skipped_mrs:
        print(f'    !{mr["iid"]} {mr["web_url"]}')
    print()
if merged_mrs:
    print('Merge requests which have been backported:')
    for mr in merged_mrs:
        print(f'    !{mr["iid"]} {mr["web_url"]}')
        print(f'        {mr["title"]}')
        print()
if manual_mrs:
    print('Merge requests which must be backported manually:')
    for (mr, reason) in manual_mrs:
        print(f'    !{mr["iid"]} {mr["web_url"]}')
        print(f'        {mr["title"]}')
        print(f'        {reason}')
        print()
