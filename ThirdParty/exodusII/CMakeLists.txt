vtk_module_third_party_internal(
  LICENSE_FILES "vtkexodusII/COPYRIGHT"
  VERSION       "7.24f"
  STANDARD_INCLUDE_DIRS)

#configure_file(
#  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exodusII.h.in"
#  "${CMAKE_CURRENT_BINARY_DIR}/vtk_exodusII.h")

vtk_module_install_headers(
#  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_exodusII.h")
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exodusII.h")
