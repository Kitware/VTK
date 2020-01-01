vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtktiff/COPYRIGHT"
    VERSION       "4.1.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE TIFF
    TARGETS TIFF::TIFF
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_tiff.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_tiff.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_tiff.h")
