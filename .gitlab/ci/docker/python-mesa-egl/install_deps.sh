#!/bin/sh

set -e

# Install extra dependencies for Mesa builds
yum install -y --setopt=install_weak_deps=False \
    meson ninja-build flex

yum clean all
