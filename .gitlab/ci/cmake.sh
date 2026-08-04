#!/bin/sh

set -e

readonly mindeps_version="3.13.5"
readonly mindeps_prefix="cmake-mindeps"
readonly latest_version="3.29.0"
readonly latest_prefix="cmake"

case "$( uname -s )-$( uname -m )" in
    Linux-x86_64)
        shatool="sha256sum"
        mindeps_sha256sum="e2fd0080a6f0fc1ec84647acdcd8e0b4019770f48d83509e6a5b0b6ea27e5864"
        mindeps_platform="Linux-x86_64"
        latest_sha256sum="f06258f52c5649752dfb10c4c2e1d8167c760c8826f078c6f5c332fa9d976bf8"
        latest_platform="linux-x86_64"
        ;;
    Linux-aarch64)
        shatool="sha256sum"
        mindeps_sha256sum="UNSUPPORTED"
        mindeps_platform="UNSUPPORTED"
        latest_sha256sum="2cb768a14b28a4478bb931d917dbc419305b82433bdecc046df98e7c336225fa"
        latest_platform="linux-aarch64"
        ;;
    Darwin-*)
        shatool="shasum -a 256"
        mindeps_sha256sum="e04bcd52c64c2ee44f6def2ac4d5a610f01b329d0db65aecc2bb5135602ebfe0"
        mindeps_platform="Darwin-x86_64"
        latest_sha256sum="4adc9d97242e18b47a4c4fa4c7d09200e4b9550cba6116e4522a7525639f436a"
        latest_platform="macos-universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly mindeps_sha256sum
readonly mindeps_platform
readonly latest_sha256sum
readonly latest_platform


# Select the CMake version to install
readonly cmake_version="${1:-latest}"

case "$cmake_version" in
    latest)
        version="$latest_version"
        sha256sum="$latest_sha256sum"
        platform="$latest_platform"
        prefix="$latest_prefix"
        ;;
    mindeps)
        version="$mindeps_version"
        sha256sum="$mindeps_sha256sum"
        platform="$mindeps_platform"
        prefix="$mindeps_prefix"

        # Skip if we're not in a `mindeps` job.
        if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'mindeps'; then
            exit 0
        fi
        ;;
    *)
        echo "Unknown CMake version: $cmake_version"
        exit 1
esac
readonly version
readonly sha256sum
readonly platform
readonly prefix

readonly filename="cmake-$version-$platform"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" "$prefix"

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin "$prefix/bin"
fi
