#!/bin/sh

set -e

if [ "$( uname -m )" != "x86_64" ]; then
    exit 0
fi

# Install tools to manage repositories.
dnf install -y --setopt=install_weak_deps=False \
    'dnf-command(config-manager)'

# Install the CUDA repository.
dnf config-manager addrepo \
    --from-repofile=https://developer.download.nvidia.com/compute/cuda/repos/fedora42/x86_64/cuda-fedora42.repo

# CUDA toolchain
dnf install -y --setopt=install_weak_deps=False \
    cuda-compiler-13-0 cuda-cudart-devel-13-0 cuda-toolkit-13-0

dnf clean all
