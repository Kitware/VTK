vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo tiff_repo
    tag  tiff_tag
  VERSION_FROM_TAG
    tag  tiff_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtktiff/LICENSE.md"
    SPDX_LICENSE_IDENTIFIER
      "libtiff"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 1988-1997 Sam Leffler"
      "Copyright (c) 1991-1997 Silicon Graphics, Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+${tiff_repo}@${tiff_tag}"
    VERSION
      "${tiff_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE TIFF
    TARGETS TIFF::TIFF
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_tiff.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_tiff.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_tiff.h")
