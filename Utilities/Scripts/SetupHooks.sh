#!/usr/bin/env bash

# Run this script to set up the git hooks for committing changes to VTK.
# For more information, see:
#   http://www.vtk.org/Wiki/VTK/Git#Hooks
#   http://www.vtk.org/Wiki/Git/Hooks

die() {
        echo 'Failure during hook setup' 1>&2
        echo '-------------------------' 1>&2
        echo '' 1>&2
        echo "$@" 1>&2
        exit 1
}

u=$(cd "$(echo "$0"|sed 's/[^/]*$//')"; pwd)
cd "$u/../../.git/hooks"

# We need to have a git repository to do a pull.
if ! test -d ./.git; then
  git init || die "Could not run git init."
fi

# Grab the hooks.
# Use the local hooks if possible.
echo "Pulling the hooks..."
if GIT_DIR=.. git for-each-ref refs/remotes/origin/hooks 2>/dev/null | \
  grep -q '\<refs/remotes/origin/hooks$'; then
  git pull .. remotes/origin/hooks
else
  git pull http://vtk.org/VTK.git hooks || die "Downloading the hooks failed."
fi
cd ../..

echo "Done."