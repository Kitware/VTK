vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkexpat/COPYING"
    VERSION       "2.2.6"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE EXPAT
    TARGETS EXPAT::EXPAT
    STANDARD_INCLUDE_DIRS)

include(vtkDetectLibraryType)
vtk_detect_library_shared(vtkexpat EXPAT::EXPAT)

if (vtkexpat_is_shared)
  set(XML_STATIC 0)
else ()
  set(XML_STATIC 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_expat.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_expat.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_expat.h")
