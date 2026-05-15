#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libxml2"
readonly ownership="libxml2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libxml2.git"
readonly tag="for/vtk-202600604-2.14.6"
readonly paths="
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
html5ent.inc
HTMLparser.c
HTMLtree.c
include/libxml/*.h
include/libxml/xmlversion.h.in
include/private/*.h
include/wsockcompat.h
iso8859x.inc
libxml.h
lintmain.c
list.c
nanohttp.c
parser.c
parserInternals.c
pattern.c
relaxng.c
runsuite.c
runtest.c
runxmlconf.c
SAX2.c
schematron.c
shell.c
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
win32/win32config.h

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
