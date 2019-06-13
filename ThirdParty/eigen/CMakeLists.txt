vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkeigen/COPYING.BSD"
    VERSION       "3.3.7"
    HEADER_ONLY
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Eigen3
    TARGETS Eigen3::Eigen3
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_eigen.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_eigen.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_eigen.h")
