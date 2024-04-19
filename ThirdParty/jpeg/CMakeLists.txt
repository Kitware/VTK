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
      "git+https://gitlab.kitware.com/third-party/libjpeg-turbo.git@for/vtk-20210727-2.1.0"
    VERSION
      "2.1.0"
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
