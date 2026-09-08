#!/bin/sh

set -e

readonly conduit_repo="https://github.com/llnl/conduit.git"
readonly conduit_commit="v0.9.7"

readonly conduit_root="$HOME/conduit"
readonly conduit_src="$conduit_root/src"
readonly conduit_build_root="$conduit_root/build"

git clone -b "$conduit_commit" --recursive "$conduit_repo" "$conduit_src"

conduit_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$conduit_src/src" \
        -B "$conduit_build_root/$subdir" \
        -DBUILD_SHARED_LIBS=ON \
        -DENABLE_PYTHON=ON \
        -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$conduit_build_root/$subdir" --target install
}

# Install the Conduits outside of /usr so they don't get picked up by accident.

# MPI-less
conduit_build nompi /opt/conduit/nompi \
    -DENABLE_MPI=OFF \
    -DCMAKE_INSTALL_LIBDIR=lib

# MPICH
conduit_build mpich /opt/conduit/mpich \
    -DENABLE_MPI=ON \
    -DCMAKE_PREFIX_PATH=/usr/lib64/mpich \
    -DCMAKE_INSTALL_LIBDIR=lib

# OpenMPI
conduit_build openmpi /opt/conduit/openmpi \
    -DENABLE_MPI=ON \
    -DCMAKE_PREFIX_PATH=/usr/lib64/openmpi \
    -DCMAKE_INSTALL_LIBDIR=lib

rm -rf "$conduit_root"
