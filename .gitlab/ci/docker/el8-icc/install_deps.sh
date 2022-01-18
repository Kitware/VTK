#!/bin/sh

set -e

# Install EPEL
dnf install -y --setopt=install_weak_deps=False \
    epel-release

# Install extra dependencies for ParaView
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core git-lfs

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel utf8cpp-devel pugixml-devel libtiff-devel \
    eigen3-devel double-conversion-devel lz4-devel expat-devel glew-devel \
    hdf5-devel hdf5-mpich-devel hdf5-openmpi-devel hdf5-devel netcdf-devel \
    netcdf-mpich-devel netcdf-openmpi-devel libogg-devel libtheora-devel \
    jsoncpp-devel gl2ps-devel protobuf-devel boost-devel gdal-devel PDAL-devel \
    cgnslib-devel

# Python dependencies
dnf install -y --setopt=install_weak_deps=False \
    python3 python3-devel python3-numpy \
    python3-pip python3-mpi4py-mpich python3-mpi4py-openmpi python3-matplotlib

# wslink will bring aiohttp>=3.7.4
python3 -m pip install 'wslink>=1.0.4'

# Upgrade libarchive (for CMake)
dnf upgrade -y --setopt=install_weak_deps=False \
    libarchive

dnf clean all
