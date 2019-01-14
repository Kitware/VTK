# FIXME: xdmf2 only exports symbols properly on Windows.
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)

vtk_module_third_party_internal(
  LICENSE_FILES "vtkxdmf2/Copyright.txt"
  VERSION       "1.2.11"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_xdmf2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_xdmf2.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_xdmf2.h")
