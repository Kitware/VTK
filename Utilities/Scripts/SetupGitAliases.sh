#!/usr/bin/env bash

# Set up some useful git aliases, global aliases for general things
echo -n "Would you like general Git aliases to be global? [y/N]:"
read answer

if [ "$answer" == "y" ]; then
  global="--global"
elif [ "$answer" == "yes" ]; then
  global="--global"
elif [ "$answer" == "Yes" ]; then
  global="--global"
else
  global=""
fi

GIT=git

GITCONFIG="${GIT} config ${global}"

# General aliases that could be global
${GITCONFIG} alias.pullall "!sh -c \"git pull && git submodule update --init\""
${GITCONFIG} alias.prepush 'log --graph --stat origin/master..'

# Staging aliases
GITCONFIG="${GIT} config"
stage_cmd='ssh git@vtk.org stage VTK'
git_branch="\$(git symbolic-ref HEAD | sed -e 's|^refs/heads/||')"
${GITCONFIG} alias.stage-cmd "!${stage_cmd}"
${GITCONFIG} alias.stage-push "!sh -c \"git fetch stage --prune && git push stage HEAD\""
${GITCONFIG} alias.stage-branch "!sh -c \"${stage_cmd} print\""
${GITCONFIG} alias.stage-merge-next "!sh -c \"${stage_cmd} merge -b next ${git_branch}\""
${GITCONFIG} alias.stage-merge "!sh -c \"${stage_cmd} merge ${git_branch}\""
# Alias to push the current topic branch to Gerrit
${GITCONFIG} alias.gerrit-push "!sh -c \"git push gerrit HEAD:refs/for/master/${git_branch}\""
