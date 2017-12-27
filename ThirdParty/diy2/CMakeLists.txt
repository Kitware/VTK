vtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES "vtkdiy2/LICENSE.txt"
                "vtkdiy2/LEGAL.txt"
  VERSION       "3.5.0")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_diy2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_diy2.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_diy2.h")
