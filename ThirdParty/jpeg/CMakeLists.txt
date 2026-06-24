vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo jpeg_repo
    tag  jpeg_tag
  VERSION_FROM_TAG
    tag  jpeg_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkjpeg/LICENSE.md"
    SPDX_LICENSE_IDENTIFIER
      "IJG AND BSD-3-Clause AND ZLIB"
    SPDX_COPYRIGHT_TEXT
      "Copyright (C)2009-2021 D. R. Commander."
      "Copyright (C)2015 Viktor Szathmáry."
      "copyright (C) 1991-2020, Thomas G. Lane, Guido Vollbeding"
    SPDX_DOWNLOAD_LOCATION
      "git+${jpeg_repo}@${jpeg_tag}"
    VERSION
      "${jpeg_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE JPEG
    TARGETS JPEG::JPEG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_jpeg.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_jpeg.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_jpeg.h")
