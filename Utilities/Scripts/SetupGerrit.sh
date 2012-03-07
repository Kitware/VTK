#!/usr/bin/env bash

# Run this script to set up the git repository to push to the Gerrit code
# review system.

die() {
  echo 1>&2 "$@" ; exit 1
}

# Make sure we are inside the repository.
cd "${BASH_SOURCE%/*}" &&

# Get current gerrit push URL.
pushurl=$(git config --get remote.gerrit.pushurl ||
          git config --get remote.gerrit.url || echo '') &&

# Tell user about current configuration.
if test -n "$pushurl"; then
  echo 'Remote "gerrit" is currently configured to push to

  '"$pushurl"'
' &&
  read -ep 'Reconfigure Gerrit? [y/N]: ' ans &&
  if [ "$ans" == "y" ] || [ "$ans" == "Y" ]; then
    setup=1
  else
    setup=''
  fi
else
  echo 'Remote "gerrit" is not yet configured.

VTK changes must be pushed to our Gerrit Code Review site:

  http://review.source.kitware.com/p/VTK

Register a Gerrit account and select a username (used below).
You will need an OpenID:

  http://openid.net/get-an-openid/
' &&
  read -ep 'Configure Gerrit? [Y/n]: ' ans &&
  if [ "$ans" == "n" ] || [ "$ans" == "N" ]; then
    exit 0
  else
    setup=1
  fi
fi &&

# Perform setup if necessary.
if test -n "$setup"; then
  echo 'Sign-in to Gerrit to get/set your username at

  http://review.source.kitware.com/#/settings

Add your SSH public keys at

  http://review.source.kitware.com/#/settings/ssh-keys
' &&
  read -ep "Gerrit username? [$USER]: " gu &&
  if test -z "$gu"; then
    gu="$USER"
  fi &&
  fetchurl='http://review.source.kitware.com/p/VTK' &&
  if test -z "$pushurl"; then
    git remote add gerrit "$fetchurl"
  else
    git config remote.gerrit.url "$fetchurl"
  fi &&
  pushurl="$gu@review.source.kitware.com:VTK" &&
  git config remote.gerrit.pushurl "$pushurl" &&
  echo 'Remote "gerrit" is now configured to push to

  '"$pushurl"'
'
fi &&

# Optionally test Gerrit access.
if test -n "$pushurl"; then
  read -ep 'Test access to Gerrit (SSH)? [y/N]: ' ans &&
  if [ "$ans" == "y" ] || [ "$ans" == "Y" ]; then
    echo -n 'Testing Gerrit access by SSH...'
    if git ls-remote --heads "$pushurl" >/dev/null; then
      echo 'passed.'
    else
      echo 'failed.' &&
      die 'Could not access Gerrit.  Add your SSH public keys at

  http://review.source.kitware.com/#/settings/ssh-keys
'
    fi
  fi
fi &&

# Set up GerritId hook.
hook=$(git config --get hooks.GerritId || echo '') &&
if test -z "$hook"; then
  echo '
Enabling GerritId hook to add a "Change-Id" footer to commit
messages for interaction with Gerrit.  Run

  git config hooks.GerritId false

to disable this feature (but you will be on your own).' &&
  git config hooks.GerritId true
else
  echo 'GerritId hook already configured to "'"$hook"'".'
fi
