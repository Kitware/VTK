vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkjpeg/LICENSE.md"
    VERSION       "2.0.3"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE JPEG
    TARGETS JPEG::JPEG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_jpeg.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_jpeg.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_jpeg.h")
