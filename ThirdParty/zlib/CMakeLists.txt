vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkzlib/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "Zlib"
    SPDX_COPYRIGHT_TEXT
      "(C) 1995-2024 Jean-loup Gailly and Mark Adler"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/zlib.git@for/vtk-20240416-1.3.1"
    VERSION
      "1.3.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE ZLIB
    TARGETS ZLIB::ZLIB
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_zlib.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_zlib.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_zlib.h")
