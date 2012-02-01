#!/usr/bin/env bash

# Centralize project variables for each script
project="VTK"
projectUrl="vtk.org"

GIT=git
GITCONFIG="${GIT} config"

# General aliases that could be global
${GITCONFIG} alias.pullall "!sh -c \"git pull && git submodule update --init\""
${GITCONFIG} alias.prepush 'log --graph --stat origin/master..'

# Staging aliases
stage_deprecated="The stage aliases have been deprecated. Please use Gerrit."
${GITCONFIG} alias.stage-cmd "!sh -c \"echo ${stage_deprecated}\""
${GITCONFIG} alias.stage-push "!sh -c \"echo ${stage_deprecated}\""
${GITCONFIG} alias.stage-branch "!sh -c \"echo ${stage_deprecated}\""
${GITCONFIG} alias.stage-merge "!sh -c \"echo ${stage_deprecated}\""
# Alias to push the current topic branch to Gerrit
${GITCONFIG} alias.gerrit-push "!bash Utilities/Scripts/git-gerrit-push"
