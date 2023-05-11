vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkzlib/LICENSE"
    VERSION       "1.2.13"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE ZLIB
    TARGETS ZLIB::ZLIB
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_zlib.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_zlib.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_zlib.h")
