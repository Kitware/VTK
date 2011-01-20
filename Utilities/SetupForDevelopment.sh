#!/usr/bin/env bash

# Run this script to set up development with git.
die() {
  echo 'Failure during git development setup' 1>&2
  echo '------------------------------------' 1>&2
  echo '' 1>&2
  echo "$@" 1>&2
  exit 1
}

# Make sure we are inside the repository.
cd "$(echo "$0"|sed 's/[^/]*$//')"/..

if test -d .git/.git; then
  die "The directory '.git/.git' exists, indicating a configuration error.

Please 'rm -rf' this directory."
fi

echo "Configuring push urls..."
git config remote.origin.pushurl git@vtk.org:VTK.git

# Rebase master by default
git config rebase.stat true
git config branch.master.rebase true

cd Utilities/Scripts

echo "Checking basic user information..."
./SetupUser.sh || exit 1
echo

echo "Setting up git hooks..."
./SetupHooks.sh || exit 1
echo

echo "Setting up the topic stage..."
./SetupTopicStage.sh || exit 1
echo

echo "Setting up git aliases..."
./SetupGitAliases.sh || exit 1
echo

echo "Setting up Gerrit..."
./SetupGerrit.sh || exit 1
echo

echo "Suggesting git tips..."
./GitTips.sh || exit 1
echo
