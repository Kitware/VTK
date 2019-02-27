vtk_module_third_party_internal(
    LICENSE_FILES "vtkloguru/LICENSE"
    VERSION       "2.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_loguru.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_loguru.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_loguru.h")
