vtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES
      "vtkpegtl/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2007-2020 Dr. Colin Hirsch and Daniel Frey"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/pegtl.git@for/vtk-20201001-2.8.3"
    VERSION
      "2.8.3"
  EXTERNAL
    PACKAGE PEGTL
    VERSION 2.0.0
    TARGETS PEGTL::PEGTL
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_pegtl.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_pegtl.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_pegtl.h")
