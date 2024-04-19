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
      "git+https://gitlab.kitware.com/third-party/tiff.git@for/vtk-20231025-4.6.0"
    VERSION
      "4.6.0"
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
