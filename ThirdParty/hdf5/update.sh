#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="hdf5"
readonly ownership="HDF Group <hdf-forum@hdfgroup.org>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://git.hdfgroup.org/scm/hdffv/hdf5.git"
readonly tag="hdf5_1_8_13"
readonly paths="
ACKNOWLEDGMENTS
CTestConfig.cmake
README.txt
CMakeLists.txt
COPYING
src
c++/CMakeLists.txt
c++/COPYING
c++/src
hl/CMakeLists.txt
hl/COPYING
hl/src
hl/c++/CMakeLists.txt
hl/c++/COPYING
hl/c++/src
config/cmake
config/lt_vers.am
CMakeInstallation.cmake
CMakeFilters.cmake
CTestConfig.cmake
UserMacros.cmake
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
