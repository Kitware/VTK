vtk_module_third_party(
  INTERNAL
    HEADER_ONLY
    LICENSE_FILES "vtkutf8/LICENSE"
    VERSION       "2.3.4"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE utf8cpp
    TARGETS utf8cpp::utf8cpp
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_utf8.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_utf8.h")
