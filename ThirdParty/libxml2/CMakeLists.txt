vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtklibxml2/Copyright"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (C) 1998-2012 Daniel Veillard"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/libxml2.git@for/vtk-20220827-2.10.1"
    VERSION
      "2.9.12"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibXml2
    TARGETS LibXml2::LibXml2
    STANDARD_INCLUDE_DIRS)

include(vtkDetectLibraryType)
vtk_detect_library_shared(vtklibxml2 LibXml2::LibXml2)

if (vtklibxml2_is_shared)
  set(LIBXML_STATIC 0)
else ()
  set(LIBXML_STATIC 1)
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_libxml2.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_libxml2.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_libxml2.h")
