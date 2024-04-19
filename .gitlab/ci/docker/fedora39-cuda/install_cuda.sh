#!/bin/sh

set -e

# Install tools to manage repositories.
dnf install -y --setopt=install_weak_deps=False \
    'dnf-command(config-manager)'

# Install the CUDA repository.
dnf config-manager --add-repo \
    https://developer.download.nvidia.com/compute/cuda/repos/fedora39/x86_64/cuda-fedora39.repo

# CUDA toolchain
dnf install -y --setopt=install_weak_deps=False \
   cuda-compiler-12-4 cuda-cudart-devel-12-4 cuda-toolkit-12-4

dnf clean all
