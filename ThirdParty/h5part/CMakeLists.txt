# we don't support external h5part
vtk_module_third_party_internal(
  LICENSE_FILES "vtkh5part/COPYING"
  VERSION       "1.6.6"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_h5part.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_h5part.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_h5part.h")
