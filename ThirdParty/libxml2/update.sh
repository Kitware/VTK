#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="libxml2"
readonly ownership="libxml2 Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/libxml2.git"
readonly tag="for/vtk-20200427-2.9.10"
readonly paths="
include/libxml/*.h
include/libxml/xmlversion.h.in

buf.c
buf.h
c14n.c
catalog.c
chvalid.c
debugXML.c
dict.c
DOCBparser.c
elfgcchack.h
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
parser.c
parserInternals.c
pattern.c
platformTestsC.c
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

config_cmake.h.in
.gitattributes
AUTHORS
CMakeLists.txt
Copyright
libxml2PlatformTests.cmake
README.kitware.md
README
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
