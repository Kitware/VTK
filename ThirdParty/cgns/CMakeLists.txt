vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkcgns/license.txt"
    VERSION       "4.1.2"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE CGNS
    VERSION       "4.1.1"
    TARGETS CGNS::CGNS
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_cgns.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_cgns.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_cgns.h")
