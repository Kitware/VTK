#!/usr/bin/env bash
set -e
set -x
shopt -s dotglob
readonly name="Twisted"
readonly ownership="Twisted Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/twisted.git"
readonly tag="for/vtk"
readonly paths="
NEWS
LICENSE
CONTRIBUTING
README.rst
.gitattributes
src/twisted/*.py
src/twisted/_threads
src/twisted/application
src/twisted/cred
src/twisted/internet/*.py
src/twisted/logger
src/twisted/persisted
src/twisted/plugins
src/twisted/protocols
src/twisted/python
src/twisted/runner
src/twisted/scripts
src/twisted/spread
src/twisted/tap
src/twisted/web
"
# src/twisted/conch
# src/twisted/enterprise
# src/twisted/mail
# src/twisted/names
# src/twisted/news
# src/twisted/pair
# src/twisted/positioning
# src/twisted/test
# src/twisted/topfiles
# src/twisted/trial
# src/twisted/words
extract_source () {
    # Copy over the files from Git
    git_archive
}
. "${BASH_SOURCE%/*}/../update-common.sh"
