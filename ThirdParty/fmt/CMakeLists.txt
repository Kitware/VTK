vtk_module_third_party(
  INTERNAL
    LICENSE_FILES           "vtkfmt/LICENSE"
    SPDX_LICENSE_IDENTIFIER "MIT"
    SPDX_COPYRIGHT_TEXT     "Copyright (c) 2012 - present, Victor Zverovich"
    SPDX_DOWNLOAD_LOCATION  "git+https://gitlab.kitware.com/third-party/fmt.git@for/vtk-20250317-11.1.4"
    VERSION                 "11.1.4"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE fmt
    TARGETS fmt::fmt
    VERSION       "11.0.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_fmt.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_fmt.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_fmt.h")
