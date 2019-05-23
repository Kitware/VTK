vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtklibproj/COPYING"
    VERSION       "4.9.3"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibPROJ
    TARGETS LibPROJ::LibPROJ
    USE_VARIABLES LibPROJ_MAJOR_VERSION
    STANDARD_INCLUDE_DIRS)

if (NOT VTK_MODULE_USE_EXTERNAL_VTK_libproj)
  set(LibPROJ_MAJOR_VERSION "4")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_libproj.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_libproj.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_libproj.h")
