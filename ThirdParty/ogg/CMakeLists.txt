vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkogg/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2002, Xiph.org Foundation"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/ogg.git@for/vtk-20210804-1.3.5"
    VERSION
      "1.3.5"
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
