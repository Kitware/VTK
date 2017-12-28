vtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES "vtkutf8/LICENSE"
  VERSION       "2.3.4")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_utf8.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")
