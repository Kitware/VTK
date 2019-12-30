#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="pugixml"
readonly ownership="Proj Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/pugixml.git"
readonly tag="for/vtk-20191226-1.10"
readonly paths="
.gitattributes
LICENSE
CMakeLists.txt
README.md
readme.txt
README.kitware.md

src/pugiconfig.hpp.in
src/pugixml.cpp
src/pugixml.hpp
src/vtk_pugixml_mangle.h
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
