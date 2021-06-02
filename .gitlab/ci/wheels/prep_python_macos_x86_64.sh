#!/bin/sh

set -e

readonly version="$1"
shift

if ! [ -d "relocatable-python" ]; then
    git clone "https://github.com/gregneagle/relocatable-python"
fi
cd relocatable-python
git pull

readonly dirname="python-$version-macos-x86_64"

touch requirements.txt
./make_relocatable_python_framework.py \
    --destination="$dirname" \
    --python-version="$version" \
    --pip-requirements=requirements.txt

tar cJf "$dirname.tar.xz" "$dirname"
