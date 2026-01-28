vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo ogg_repo
    tag  ogg_tag
  VERSION_FROM_TAG
    tag  ogg_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkogg/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2002, Xiph.org Foundation"
    SPDX_DOWNLOAD_LOCATION
      "git+${ogg_repo}@${ogg_tag}"
    VERSION
      "${ogg_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE OGG
    TARGETS OGG::OGG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_ogg.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_ogg.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_ogg.h")
