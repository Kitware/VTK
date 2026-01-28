# note: the CGNS version is determined by contracting the minor and bugfix version
#       into a single version; 4.2.0 becomes 4.20. See src/cgnslib.h CGNS_DOTVERS constant.
# Furthermore, 4.1.x versions have been released with version 4.10, even when x > 0.
#
# For more information see:
# https://gitlab.kitware.com/vtk/vtk/-/issues/18654
# https://github.com/CGNS/CGNS/discussions/357

vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo cgns_repo
    tag  cgns_tag
  VERSION_FROM_TAG
    tag  cgns_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkcgns/license.txt"
    SPDX_LICENSE_IDENTIFIER
      "Zlib"
    SPDX_COPYRIGHT_TEXT
      "© Copyright 1991-2020, CGNS Steering Commmittee"
    SPDX_DOWNLOAD_LOCATION
      "git+${cgns_repo}@${cgns_tag}"
    VERSION
      "${cgns_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE CGNS
    VERSION "4.1.0"
    TARGETS CGNS::CGNS
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_cgns.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_cgns.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_cgns.h")
