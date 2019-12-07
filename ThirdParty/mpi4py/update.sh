#!/usr/bin/env bash

# This import runs Cython which has historically tended to include absolute
# paths into the generated files (which we use to avoid a dependency on Cython
# in VTK itself). No release has been made as of 20 April 2017, but the patch
# in this pull request makes the build reproducible:
#
#     https://github.com/cython/cython/pull/1576

set -e
set -x
shopt -s dotglob

readonly name="mpi4py"
readonly ownership="mpi4py Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/mpi4py.git"
readonly tag="for/vtk-20191127-3.0.1"
readonly paths="
.gitattributes
CMakeLists.txt
LICENSE.rst
README.rst
CHANGES.rst
misc/THANKS.txt
src/CMakeLists.txt
src/lib-mpi/
src/*.h
src/MPI.c
src/mpi4py/include/
src/mpi4py/__init__.py
src/mpi4py/__init__.pxd
src/mpi4py/libmpi.pxd
src/mpi4py/MPI.pxd
"

extract_source () {
    # Run cython
    python2 setup.py build_src
    # Copy over the files from Git
    git_archive
    # Copy over the files cython produced
    cp -v "src/mpi4py/include/mpi4py/mpi4py.MPI.h" "$extractdir/$name-reduced/src/mpi4py/include/mpi4py/"
    cp -v "src/mpi4py/include/mpi4py/mpi4py.MPI_api.h" "$extractdir/$name-reduced/src/mpi4py/include/mpi4py/"
    cp -v "src/mpi4py.MPI.c" "$extractdir/$name-reduced/src/"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
