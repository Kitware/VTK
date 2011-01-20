#!/usr/bin/env bash

# This script makes optional suggestions for working with git.
if test "$(git config color.ui)" != "auto"; then
  cat << EOF

You may want to enable color output from Git commands with

  git config --global color.ui auto

EOF
fi

if ! bash -i -c 'echo $PS1' | grep -q '__git_ps1'; then
  cat << EOF

A dynamic, informative Git shell prompt can be obtained by sourcing the git
bash-completion script in your ~/.bashrc.  Set the PS1 environmental variable as
suggested in the comments at the top of the bash-completion script.  You may
need to install the bash-completion package from your distribution to obtain the
script.

EOF
fi

if ! git config merge.tool >/dev/null; then
  cat << EOF

A merge tool can be configured with

  git config merge.tool <toolname>

For more information, see

  git help mergetool

EOF
fi

echo "Done."
