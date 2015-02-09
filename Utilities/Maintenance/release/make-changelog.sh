#!/bin/sh

die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -lt 2 ] && \
    die "usage: $0 <old release> <new release>"

readonly old="$1"
shift

readonly new="$1"
shift

exec git shortlog -e --no-merges --format="%h %s" "${old}..${new}" > changes.txt
