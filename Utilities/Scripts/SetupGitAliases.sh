#!/usr/bin/env bash

echo "Setting up useful Git aliases..." &&

# General aliases that could be global
git config alias.pullall '!bash -c "git pull && git submodule update --init"' &&
git config alias.prepush 'log --graph --stat origin/master..' &&

# Staging aliases
stage_disabled="VTK no longer uses the topic stage. Please use GitLab." &&
git config alias.stage-cmd '!sh -c "echo '"${stage_disabled}"'"' &&
git config alias.stage-push '!sh -c "echo '"${stage_disabled}"'"' &&
git config alias.stage-branch '!sh -c "echo '"${stage_disabled}"'"' &&
git config alias.stage-merge '!sh -c "echo '"${stage_disabled}"'"' &&

# Gerrit aliases
gerrit_disabled="VTK no longer uses Gerrit. Please use GitLab." &&
git config alias.gerrit-push '!sh -c "echo '"${gerrit_disabled}"'"' &&

# Alias to push the current topic branch to GitLab
git config alias.gitlab-push '!bash Utilities/GitSetup/git-gitlab-push' &&

true
