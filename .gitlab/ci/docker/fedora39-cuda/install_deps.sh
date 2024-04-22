#!/bin/sh

set -e

# Install extra dependencies for VTK
dnf install -y --setopt=install_weak_deps=False \
    bzip2 patch git-core git-lfs

# Development tools
dnf install -y --setopt=install_weak_deps=False \
    ninja-build

# MPI dependencies
dnf install -y --setopt=install_weak_deps=False \
    openmpi-devel mpich-devel

# Mesa dependencies
dnf install -y --setopt=install_weak_deps=False \
    mesa-libOSMesa-devel mesa-libOSMesa mesa-dri-drivers mesa-libGL* glx-utils

# External dependencies
dnf install -y --setopt=install_weak_deps=False \
    libXcursor-devel freeglut-devel

dnf clean all
