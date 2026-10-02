vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo nanoflann_repo
    tag  nanoflann_tag
  VERSION_FROM_TAG
    tag  nanoflann_version "^for/vtk-[0-9]*-([0-9\\.]+)$"
  )

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtknanoflann/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "BSD-2-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2008-2009 Marius Muja"
      "Copyright 2008-2009 David G. Lowe"
      "Copyright 2011-2022 Jose Luis Blanco"
    SPDX_DOWNLOAD_LOCATION
      "git+${nanoflann_repo}@${nanoflann_tag}"
    VERSION
      "${nanoflann_version}"
    HEADER_ONLY
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE nanoflann
    TARGETS nanoflann::nanoflann
    VERSION "1.14.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_nanoflann.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_nanoflann.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_nanoflann.h")
