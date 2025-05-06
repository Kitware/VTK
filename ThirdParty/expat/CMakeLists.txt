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
      "git+https://gitlab.kitware.com/third-party/expat.git@for/vtk-20250317-2.7.0"
    VERSION
    "2.7.0"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE EXPAT
    VERSION 2.5.0
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

if(NOT DEFINED ENV{DASHBOARD_TEST_FROM_CTEST})
  # Check for deprecated versions of expat
  set(expat_deprecated_version "2.6.3")
  if(VTK_MODULE_USE_EXTERNAL_vtkexpat AND EXPAT_VERSION_STRING VERSION_LESS expat_deprecated_version)
    message(DEPRECATION
      "External expat ${EXPAT_VERSION_STRING} is less than ${expat_deprecated_version} "
      "and (may) have known vulnerabilities. Consider upgrading.")
  endif()
endif()
