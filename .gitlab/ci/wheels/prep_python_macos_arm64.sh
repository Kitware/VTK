#!/bin/sh

set -e

readonly version="$1"
readonly freethreading="$2"

maybe_free_threading=""
if [ "$freethreading" = "-t" ]; then
  maybe_free_threading="t"
fi

if ! [ -d "relocatable-python" ]; then
    git clone "https://github.com/gregneagle/relocatable-python"
fi
cd relocatable-python
git pull

readonly dirname="python-${version}${maybe_free_threading}-macos-arm64"

touch requirements.txt
./make_relocatable_python_framework.py \
    --destination="$dirname" \
    --python-version="$version" \
    --os-version="11" \
    --pip-requirements=requirements.txt

tar cJf "$dirname.tar.xz" "$dirname"
