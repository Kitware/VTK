#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libxml2"
readonly ownership="libxml2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libxml2.git"
readonly tag="for/vtk-20260922-2.15.4"
readonly paths="
buf.c
c14n.c
catalog.c
chvalid.c
codegen/charset.inc
codegen/escape.inc
codegen/html5ent.inc
codegen/ranges.inc
codegen/unicode.inc
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
xmlwriter.c
xpath.c
xpointer.c

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
