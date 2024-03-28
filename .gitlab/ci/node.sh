#!/bin/sh

set -e

readonly version="20.11.1"
readonly prefix="node"

# node is only used on linux-x86_64 runners.
case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        platform="linux"
        case "$( uname -m )" in
            x86_64)
                architecture="x64"
                sha256sum="bf3a779bef19452da90fb88358ec2c57e0d2f882839b20dc6afc297b6aafc0d7"
                ;;
            *)
                echo "Unrecognized architecture $( uname -m )"
                exit 1
                ;;
        esac
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform
readonly architecture

readonly filename="node-v$version-$platform-$architecture"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > node.sha256sum
curl -OL "https://nodejs.org/download/release/v$version/$tarball"
$shatool --check node.sha256sum
./cmake/bin/cmake -E tar xf "$tarball"
mv "$filename" "$prefix"
