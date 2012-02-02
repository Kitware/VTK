#!/usr/bin/env bash

# This script makes optional suggestions for working with git.

egrep-q() {
  egrep "$@" >/dev/null 2>/dev/null
}

if test -z "$(git config --get color.ui)"; then
  echo '
One may enable color output from Git commands with

  git config --global color.ui auto
'
fi

if ! bash -i -c 'echo $PS1' | egrep-q '__git_ps1'; then
  echo '
A dynamic, informative Git shell prompt can be obtained by sourcing the git
bash-completion script in your ~/.bashrc.  Set the PS1 environmental variable as
suggested in the comments at the top of the bash-completion script.  You may
need to install the bash-completion package from your distribution to obtain it.
'
fi

if test -z "$(git config --get merge.tool)"; then
  echo '
One may configure Git to load a merge tool with

  git config merge.tool <toolname>

See "git help mergetool" for more information.
'
fi
