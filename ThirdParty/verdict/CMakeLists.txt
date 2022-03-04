vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkverdict/LICENSE"
    VERSION       "1.4.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Verdict
    TARGETS Verdict::verdict
    VERSION       "1.4.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_verdict.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_verdict.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_verdict.h")
