vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkioss/COPYRIGHT"
    VERSION       "2022-10-14"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE SEACASIoss
    TARGETS Ioss
    USE_VARIABLES SEACASIoss_INCLUDE_DIRS
    STANDARD_INCLUDE_DIRS)

if (VTK_MODULE_USE_EXTERNAL_vtkioss)
  # The usage requirements for Ioss' exported targets is not useful for us.
  # https://github.com/gsjaardema/seacas/issues/239
  vtk_module_include(VTK::ioss
    INTERFACE
      "${SEACASIoss_INCLUDE_DIRS}")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_ioss.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_ioss.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_ioss.h")
