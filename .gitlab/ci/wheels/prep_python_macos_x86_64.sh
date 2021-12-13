#!/bin/sh

set -e

readonly version="$1"
shift

if ! [ -d "relocatable-python" ]; then
    git clone "https://github.com/gregneagle/relocatable-python"
fi
cd relocatable-python
git pull

# The prebuilt binaries on python.org target different macOS versions based on
# the Python version. Use this to instruct which binary to download for the
# `relocatable-python` project scripts.
case "$version" in
    3.?.*)
        os_version="10.9"
        ;;
    3.1?.*)
        os_version="11"
        ;;
esac
readonly os_version

readonly dirname="python-$version-macos-x86_64"

touch requirements.txt
./make_relocatable_python_framework.py \
    --destination="$dirname" \
    --python-version="$version" \
    --os-version="$os_version" \
    --pip-requirements=requirements.txt

tar cJf "$dirname.tar.xz" "$dirname"
