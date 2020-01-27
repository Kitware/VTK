#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="xdmf3"
readonly ownership="XDMF Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/xdmf.git"
readonly tag="for/vtk-20191216-master-gfe7dd1ca"
readonly paths="
.gitattributes
CMake/XdmfFunctions.cmake
CMake/VersionSuite
CMakeLists.txt
Copyright.txt
*.hpp
*.cpp
XdmfConfig.hpp.in
core/CMakeLists.txt
core/*.cpp
core/*.hpp
core/*.tpp
core/XdmfConfig.hpp.in
core/XdmfCoreConfig.hpp.in
core/loki/
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
