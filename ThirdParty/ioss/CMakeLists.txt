# FIXME: ioss never exports symbols properly
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkioss/COPYRIGHT"
    VERSION       "2021-05-12"
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
