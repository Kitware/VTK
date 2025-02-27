#!/bin/sh

set -e

# Install extra dependencies for Mesa builds
yum install -y --setopt=install_weak_deps=False \
    meson flex

yum clean all

readonly version="1.12.1"

case "$( uname -m )" in
    x86_64)
        sha256sum="6f98805688d19672bd699fbbfa2c2cf0fc054ac3df1f0e6a47664d963d530255"
        platform="linux"
        ;;
    aarch64)
        sha256sum="5c25c6570b0155e95fce5918cb95f1ad9870df5768653afe128db822301a05a1"
        platform="linux-aarch64"
        ;;
    *)
        echo "Unrecognized architecture $( uname -m )"
        exit 1
        ;;
esac
readonly sha256sum
readonly platform

readonly filename="ninja-$platform"
readonly tarball="$filename.zip"

echo "$sha256sum  $tarball" > ninja.sha256sum
curl -OL "https://github.com/ninja-build/ninja/releases/download/v$version/$tarball"
sha256sum --check ninja.sha256sum
python3 -c "
import zipfile
with zipfile.ZipFile('$tarball', 'r') as zip_ref:
    zip_ref.extractall('/usr/local/bin')
"
chmod +x /usr/local/bin/ninja
