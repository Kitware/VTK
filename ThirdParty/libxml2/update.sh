#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libxml2"
readonly ownership="libxml2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libxml2.git"
readonly tag="for/vtk-20220827-2.10.1"
readonly paths="
include/libxml/*.h
include/libxml/xmlversion.h.in
include/win32config.h

configure.ac
buf.c
buf.h
c14n.c
catalog.c
chvalid.c
config.h.cmake.in
debugXML.c
dict.c
enc.h
encoding.c
entities.c
error.c
globals.c
hash.c
HTMLparser.c
HTMLtree.c
legacy.c
libxml.h
list.c
nanoftp.c
nanohttp.c
parser.c
parserInternals.c
pattern.c
relaxng.c
SAX.c
SAX2.c
save.h
schematron.c
threads.c
timsort.h
tree.c
triodef.h
trionan.c
trionan.h
uri.c
valid.c
xinclude.c
xlink.c
xmlIO.c
xmlmemory.c
xmlmodule.c
xmlreader.c
xmlregexp.c
xmlsave.c
xmlschemas.c
xmlschemastypes.c
xmlstring.c
xmlunicode.c
xmlwriter.c
xpath.c
xpointer.c
xzlib.c

win32/libxml2.rc

.gitattributes
CMakeLists.txt
Copyright
README.kitware.md
README.md
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
