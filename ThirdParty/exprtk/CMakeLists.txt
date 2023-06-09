vtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES           "vtkexprtk/License.txt"
    SPDX_LICENSE_IDENTIFIER "MIT"
    SPDX_COPYRIGHT_TEXT     "Arash Partow (1999-2021)"
    SPDX_DOWNLOAD_LOCATION  "git+https://gitlab.kitware.com/third-party/exprtk.git@for/vtk-20210816-2.71"
    VERSION                 "2.71"
  EXTERNAL
    PACKAGE ExprTk
    VERSION 2.7
    TARGETS ExprTk::ExprTk
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exprtk.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_exprtk.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_exprtk.h")
