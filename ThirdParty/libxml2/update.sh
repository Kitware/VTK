#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libxml2"
readonly ownership="libxml2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libxml2.git"
readonly tag="for/vtk-20250703-2.13.5"
readonly paths="

configure.ac
buf.c
c14n.c
catalog.c
chvalid.c
config.h.cmake.in
debugXML.c
dict.c
encoding.c
entities.c
error.c
globals.c
hash.c
HTMLparser.c
HTMLtree.c
include/libxml/*.h
include/libxml/xmlversion.h.in
include/private/*.h
include/win32config.h
include/wsockcompat.h
legacy.c
libxml.h
list.c
nanoftp.c
nanohttp.c
parser.c
parserInternals.c
pattern.c
relaxng.c
rngparser.c
runsuite.c
runtest.c
runxmlconf.c
SAX2.c
SAX.c
schematron.c
threads.c
timsort.h
tree.c
uri.c
valid.c
xinclude.c
xlink.c
xmlcatalog.c
xmlIO.c
xmllint.c
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
