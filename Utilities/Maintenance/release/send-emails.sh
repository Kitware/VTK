#!/bin/sh

die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -lt 1 ] && \
    die "usage: $0 <version>"

readonly version="$1"
shift

[ -z "$mailer" ] && \
    mailer=mail

subject="summarize your vtk $version changes please"
maintainers="-c ben.boeckel@kitware.com -c dave.demarle@kitware.com -c chuck.atkins@kitware.com"

for email in *@*.txt; do
    address="$( basename "$email" .txt )"
    $mailer -s "$subject" $maintainers "$address" < $email
done
