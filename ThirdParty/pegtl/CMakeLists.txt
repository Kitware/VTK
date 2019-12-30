vtk_module_third_party_internal(
  HEADER_ONLY
  LICENSE_FILES "vtkpegtl/LICENSE"
  VERSION       "2.8.1")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_pegtl.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_pegtl.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_pegtl.h")
