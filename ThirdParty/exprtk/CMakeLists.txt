vtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES           "vtkexprtk/license.txt"
    SPDX_LICENSE_IDENTIFIER "MIT"
    SPDX_COPYRIGHT_TEXT     "1999-2024 Arash Partow"
    SPDX_DOWNLOAD_LOCATION  "git+https://gitlab.kitware.com/third-party/exprtk.git@for/vtk-20250313-0.0.3-cmake"
    VERSION                 "0.0.3"
  EXTERNAL
    PACKAGE ExprTk
    VERSION 0.0.3
    TARGETS ExprTk::ExprTk
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exprtk.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_exprtk.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_exprtk.h")
