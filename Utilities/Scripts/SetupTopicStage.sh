#!/usr/bin/env bash

# Run this script to set up the topic stage for pushing changes.
die() {
  echo 'Failure during topic stage setup.' 1>&2
  echo '---------------------------------' 1>&2
  echo '' 1>&2
  echo "$@" 1>&2
  exit 1
}

# Centralize project variables for each script
project="VTK"
projectUrl="vtk.org"

# Make sure we are inside the repository.
cd "$(echo "$0"|sed 's/[^/]*$//')"

if git config remote.stage.url >/dev/null; then
  echo "Topic stage remote was already configured."
else
  echo "Configuring the topic stage remote..."
  git remote add stage git://${projectUrl}/stage/${project}.git || \
    die "Could not add the topic stage remote."
  git config remote.stage.pushurl git@${projectUrl}:stage/${project}.git
fi

read -ep "Do you want to test push access for ${project}? [y/N]: " access
if [ "$access" == "y" ] || [ "$access" == "Y" ]; then

  echo "Configuring push urls..."
  if [ "`git config remote.origin.url`" == "git://${projectUrl}/${project}.git" ]; then
    git config remote.origin.pushurl git@${projectUrl}:${project}.git
  fi

  echo "Testing ssh capabilities..."
  git ls-remote git@${projectUrl}:stage/${project}.git refs/heads/master || \
    die "SSH test to git@${projectUrl} failed. You may need to request access at:

https://www.kitware.com/Admin/SendPassword.cgi

Note that push access to the stage/${project} is separate to Gerrit.
"

  echo "Test successful! ${project} push access confirmed. Summary of project access:"
  echo
  # This command queries gitolite for your access privileges
  ssh git@${projectUrl} info
fi

echo "Done."
