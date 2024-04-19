#!/bin/sh

set -e

# Skip for non-tidy builds.
if ! echo "$CMAKE_CONFIGURATION" | grep -q -e 'tidy'; then
    exit 0
fi

readonly version="0.4.0-kitware-ci-20220709"

case "$( uname -s )-$(uname -m)" in
    Linux-x86_64)
        shatool="sha256sum"
        sha256sum="49c2540695da9834895897633c8323d5685df88f32b71da1ac373fff9426a063"
        platform="linux-amd64"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="clang-tidy-cache-$version-$platform"

readonly url="https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/clang-tidy-cache/v$version"

cd .gitlab

echo "$sha256sum  $filename" > clang-tidy-cache.sha256sum
curl -OL "$url/$filename"
$shatool --check clang-tidy-cache.sha256sum
mv "$filename" clang-tidy-cache
chmod +x clang-tidy-cache
