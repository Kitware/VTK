vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkexpat/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 1998-2000 Thai Open Source Software Center Ltd and Clark Cooper"
      "Copyright (c) 2001-2019 Expat maintainers"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/expat.git@for/vtk-20220921-2.4.8"
    VERSION
      "2.4.1"
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

# match setting of EXPAT_LARGE_SIZE in our build
if(NOT VTK_MODULE_USE_EXTERNAL_vtkexpat)
  set(XML_LARGE_SIZE 1)
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_expat.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_expat.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_expat.h")
