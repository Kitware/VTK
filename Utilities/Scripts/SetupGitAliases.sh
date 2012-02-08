#!/usr/bin/env bash

echo "Setting up useful Git aliases..." &&

# General aliases that could be global
git config alias.pullall "!bash -c \"git pull && git submodule update --init\"" &&
git config alias.prepush 'log --graph --stat origin/master..' &&

# Staging aliases
stage_deprecated="The stage aliases have been deprecated. Please use Gerrit." &&
git config alias.stage-cmd "!sh -c \"echo ${stage_deprecated}\"" &&
git config alias.stage-push "!sh -c \"echo ${stage_deprecated}\"" &&
git config alias.stage-branch "!sh -c \"echo ${stage_deprecated}\"" &&
git config alias.stage-merge "!sh -c \"echo ${stage_deprecated}\"" &&

# Alias to push the current topic branch to Gerrit
git config alias.gerrit-push "!bash Utilities/Scripts/git-gerrit-push" &&

true
