#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="fides"
readonly ownership="Fides Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/fides.git"
readonly tag="for/vtk-20201004-master"
readonly paths="
.gitattributes
CMakeLists.txt
fides
cmake/FidesModule.cmake
cmake/FidesExportHeaderTemplate.h.in
cmake/fides_generate_export_header.cmake
thirdparty
LICENSE.txt
README.md
README.kitware.md
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
