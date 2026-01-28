vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo lz4_repo
    tag  lz4_tag
  VERSION_FROM_TAG
    tag  lz4_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtklz4/lib/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-2-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2011-2016, Yann Collet"
    SPDX_DOWNLOAD_LOCATION
      "git+${lz4_repo}@${lz4_tag}"
    VERSION
      "${lz4_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LZ4
    TARGETS LZ4::LZ4
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_lz4.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_lz4.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_lz4.h")
