#!/usr/bin/env bash

# Run this script to set up the git repository to push to the Gerrit code review
# system.
die() {
  echo 'Failure during Gerrit setup.' 1>&2
  echo '----------------------------' 1>&2
  echo '' 1>&2
  echo "$@" 1>&2
  exit 1
}

# Centralize project variables for each script
project="VTK"

gerrit_user() {
  read -ep "Enter your gerrit user (set in Gerrit Settings/Profile) [$USER]: " gu
  if [ "$gu" == "" ]; then
   # Use current user name.
   gu=$USER
  fi
  echo -e "\nConfiguring 'gerrit' remote with user '$gu'..."
  if git config remote.gerrit.url >/dev/null; then
    # Correct the remote url
    git remote set-url gerrit $gu@review.source.kitware.com:${project} || \
      die "Could not amend gerrit remote."
  else
    # Add a new one
    git remote add gerrit $gu@review.source.kitware.com:${project} || \
      die "Could not add gerrit remote."
  fi
  cat << EOF

For more information on Gerrit usage, see

  http://public.kitware.com/Wiki/ITK/Gerrit
EOF
}

# Make sure we are inside the repository.
cd "$(echo "$0"|sed 's/[^/]*$//')"

if git config remote.gerrit.url >/dev/null; then
  echo "Gerrit was already configured. The configured remote URL is:"
  echo
  git config remote.gerrit.url
  echo
  read -ep "Is the username correct? [Y/n]: " correct
  if [ "$correct" == "n" ] || [ "$correct" == "N" ]; then
    gerrit_user
  fi
else
  cat << EOF
Gerrit is a code review system that works with Git.

In order to use Gerrit, an account must be registered at the review site:

  http://review.source.kitware.com/p/${project}

In order to register you need an OpenID

  http://openid.net/get-an-openid/

EOF
  gerrit_user
fi

read -ep "Would you like to verify authentication to Gerrit? [y/N]: " ans
if [ "$ans" == "y" ] || [ "$ans" == "Y" ]; then
  echo
  echo "Fetching from gerrit to test SSH key configuration (Settings/SSH Public Keys)"
  git fetch gerrit ||
    die "Could not fetch gerrit remote. You need to upload your public SSH key to Gerrit."
  echo "Done."
fi

echo -e "\nConfiguring GerritId hook..."
if git config hooks.GerritId >/dev/null; then
  echo "GerritId hook already configured."
else
    cat << EOF
This hook automatically add a "Change-Id" footer to commit messages
to make interaction with Gerrit easier.
To disable this feature, run

  git config hooks.GerritId false

EOF
  git config hooks.GerritId true
  echo "Done."
fi
