#!/usr/bin/env bash

# Run this script to set up the topic stage for pushing changes.

die() {
        echo 'Failure during topic stage setup' 1>&2
        echo '--------------------------------' 1>&2
        echo '' 1>&2
        echo "$@" 1>&2
        exit 1
}

# Make sure we are inside the repository.
cd "$(echo "$0"|sed 's/[^/]*$//')"

if git config remote.stage.url >/dev/null; then
  echo "Topic stage remote was already configured."
else
  echo "Configuring the topic stage remote..."
  git remote add stage git://vtk.org/stage/VTK.git || \
    die "Could not add the topic stage remote."
  git config remote.stage.pushurl git@vtk.org:stage/VTK.git
fi

read -ep "Do you have git push access to vtk.org? [y/N]: " access
if test "$access" = "y"; then
  echo "Testing ssh capabilities..."
  git ls-remote git@vtk.org:stage/VTK.git refs/heads/master || die "ssh test to git@vtk.org failed."
  echo "Test successful!"
fi

echo "Done."
