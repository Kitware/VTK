#!/usr/bin/env bash

# Run this script to set up basic user information.

die() {
  echo 'Failure during user information setup.' 1>&2
  echo '--------------------------------------' 1>&2
  echo '' 1>&2
  echo "$@" 1>&2
  exit 1
}

setup_user() {
  read -ep "Please enter your full name, such as 'John Doe': " name
  echo "Setting name to '$name'"
  git config user.name "$name"
  read -ep "Please enter your email address, such as 'john@gmail.com': " email
  echo "Setting email address to '$email'"
  git config user.email "$email"
}

# Added some logic to introduce yourself to Git.
gitName=$(git config user.name)
gitEmail=$(git config user.email)
if [ "$gitName" == "" ] || [ "$gitEmail" == "" ]; then
  setup_user
fi

# Loop until the user is happy with the authorship information
for (( ; ; ))
do
  # Display the final user information.
  gitName=$(git config user.name)
  gitEmail=$(git config user.email)
  echo "Your commits will have the following author:

  $gitName <$gitEmail>
"
  read -ep "Is the author name and email address above correct? [Y/n] " correct
  if [ "$correct" == "n" ] || [ "$correct" == "N" ]; then
    setup_user
  else
    break
  fi
done
