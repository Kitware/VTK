vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkogg/COPYING"
    VERSION       "1.3.4"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE OGG
    TARGETS OGG::OGG
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_ogg.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_ogg.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_ogg.h")
