vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo expat_repo
    tag  expat_tag
  VERSION_FROM_TAG
    tag  expat_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

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
      "git+${expat_repo}@${expat_tag}"
    VERSION
      "${expat_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE EXPAT
    VERSION 2.2.1 # absolute minimun, correspending to version in macOS 10.13
    TARGETS EXPAT::EXPAT
    STANDARD_INCLUDE_DIRS
    USE_VARIABLES EXPAT_VERSION)

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
  if(VTK_MODULE_USE_EXTERNAL_vtkexpat AND EXPAT_VERSION VERSION_LESS expat_deprecated_version)
    message(DEPRECATION
      "External expat ${EXPAT_VERSION} is less than ${expat_deprecated_version} "
      "and (may) have known vulnerabilities. Consider upgrading.")
  endif()
endif()
