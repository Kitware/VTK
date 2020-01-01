vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkfreetype/docs/FTL.TXT"
                  "vtkfreetype/docs/LICENSE.TXT"
    VERSION       "2.10.1"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Freetype
    TARGETS Freetype::Freetype
    STANDARD_INCLUDE_DIRS)

include(vtkDetectLibraryType)
vtk_detect_library_shared(vtkfreetype Freetype::Freetype)

set(VTK_FREETYPE_SHARED "${vtkfreetype_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_freetype.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_freetype.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_freetype.h")
