#!/usr/bin/env bash

# Run this script to set up the git hooks for committing changes to VTK.
# For more information, see:
#   http://www.vtk.org/Wiki/Git/Hooks

egrep-q() {
  egrep "$@" >/dev/null 2>/dev/null
}

die() {
  echo 1>&2 "$@" ; exit 1
}

echo 'Setting up git hooks...' &&
cd "${BASH_SOURCE%/*}" &&
git_dir=$(git rev-parse --git-dir) &&
cd "$git_dir/hooks" &&
if ! test -e .git; then
  git init -q || die 'Could not run git init for hooks.'
fi &&
if GIT_DIR=.. git for-each-ref refs/remotes/origin/hooks 2>/dev/null |
  egrep-q 'refs/remotes/origin/hooks$'; then
  git fetch -q .. remotes/origin/hooks
else
  git fetch -q http://vtk.org/VTK.git hooks
fi &&
git reset -q --hard FETCH_HEAD || die 'Failed to install hooks'
