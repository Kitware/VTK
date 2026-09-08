#!/bin/sh

set -e

readonly catalyst_repo="https://gitlab.kitware.com/paraview/catalyst"
readonly catalyst_commit="v2.1.0"

readonly catalyst_root="$HOME/catalyst"
readonly catalyst_src="$catalyst_root/src"
readonly catalyst_build_root="$catalyst_root/build"

git clone -b "$catalyst_commit" "$catalyst_repo" "$catalyst_src"

catalyst_build () {
    local subdir="$1"
    shift

    local prefix="$1"
    shift

    cmake -GNinja \
        -S "$catalyst_src" \
        -B "$catalyst_build_root/$subdir" \
        -DCATALYST_BUILD_SHARED_LIBS=ON \
        -DCATALYST_BUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        "-DCMAKE_INSTALL_PREFIX=$prefix" \
        "$@"
    cmake --build "$catalyst_build_root/$subdir" --target install
}

# Catalyst installs configured to use the vendored Conduit

# MPI-less
catalyst_build nompi /usr \
    -DCATALYST_USE_MPI=OFF

# MPICH
catalyst_build mpich /usr/lib64/mpich \
    -DCATALYST_USE_MPI=ON \
    -DCMAKE_INSTALL_LIBDIR=lib

# OpenMPI
catalyst_build openmpi /usr/lib64/openmpi \
    -DCATALYST_USE_MPI=ON \
    -DCMAKE_INSTALL_LIBDIR=lib

# The Catalyst installs below are configured to use an external Conduit. They
# live under /opt, rather than /usr, to prevent them getting used accidentally.
#
# Any build enabling VTK::conduit must use one of these (the installs above
# mangle their vendored Conduit).
#
# Installing outside the MPI prefixes means MPI is no longer found via
# CMAKE_INSTALL_PREFIX, so the MPI prefix has to be passed explicitly
# for these builds.

# MPI-less, external Conduit
catalyst_build nompi-ext /opt/catalyst-ext/nompi \
    -DCATALYST_WITH_EXTERNAL_CONDUIT=ON \
    -DConduit_DIR=/opt/conduit/nompi/lib/cmake/conduit \
    -DCATALYST_USE_MPI=OFF \
    -DCMAKE_INSTALL_LIBDIR=lib

# MPICH, external Conduit
catalyst_build mpich-ext /opt/catalyst-ext/mpich \
    -DCATALYST_WITH_EXTERNAL_CONDUIT=ON \
    -DConduit_DIR=/opt/conduit/mpich/lib/cmake/conduit \
    -DCATALYST_USE_MPI=ON \
    -DCMAKE_PREFIX_PATH=/usr/lib64/mpich \
    -DCMAKE_INSTALL_LIBDIR=lib

# OpenMPI, external Conduit
catalyst_build openmpi-ext /opt/catalyst-ext/openmpi \
    -DCATALYST_WITH_EXTERNAL_CONDUIT=ON \
    -DConduit_DIR=/opt/conduit/openmpi/lib/cmake/conduit \
    -DCATALYST_USE_MPI=ON \
    -DCMAKE_PREFIX_PATH=/usr/lib64/openmpi \
    -DCMAKE_INSTALL_LIBDIR=lib

rm -rf "$catalyst_root"
