vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkfreetype/docs/FTL.TXT"
      "vtkfreetype/LICENSE.TXT"
    SPDX_LICENSE_IDENTIFIER
      "FTL"
    SPDX_COPYRIGHT_TEXT
      "Copyright 1996-2002, 2006 by David Turner, Robert Wilhelm, and Werner Lemberg"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/freetype.git@for/vtk-20220414-2.12.0"
    VERSION
      "2.12.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Freetype
    TARGETS Freetype::Freetype
    STANDARD_INCLUDE_DIRS)

include(vtkDetectLibraryType)
# Detect `freetype-config` usage.
if (VTK_MODULE_USE_EXTERNAL_vtkfreetype AND TARGET freetype)
  vtk_detect_library_shared(vtkfreetype freetype)
else ()
  vtk_detect_library_shared(vtkfreetype Freetype::Freetype)
endif ()

set(VTK_FREETYPE_SHARED "${vtkfreetype_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_freetype.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_freetype.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_freetype.h")
