vtk_module_third_party(
  INTERNAL
    LICENSE_FILES 
      "vtklz4/lib/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "BSD-2-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2011-2016, Yann Collet"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/lz4.git@for/vtk-20210727-1.9.3"
    VERSION       
      "1.9.3"
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
