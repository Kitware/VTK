#!/bin/sh

set -e

# Install extra dependencies for OSMesa builds
yum install -y --setopt=install_weak_deps=False \
    meson ninja-build flex

yum clean all
