#!/bin/sh

# Install tools to manage repositories.
dnf install -y --setopt=install_weak_deps=False \
    'dnf-command(config-manager)'

# Install the CUDA repository.
dnf config-manager --add-repo \
    https://developer.download.nvidia.com/compute/cuda/repos/fedora33/x86_64/cuda-fedora33.repo

# CUDA toolchain
dnf install -y --setopt=install_weak_deps=False \
   cuda-compiler-11-2 cuda-cudart-devel-11-2 cuda-toolkit-11-2

dnf clean all
