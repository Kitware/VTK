vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo pegtl_repo
    tag  pegtl_tag
  VERSION_FROM_TAG
    tag  pegtl_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

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
      "git+${pegtl_repo}@${pegtl_tag}"
    VERSION
      "${pegtl_version}"
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
