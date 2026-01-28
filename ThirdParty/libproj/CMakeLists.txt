vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo libproj_repo
    tag  libproj_tag
  VERSION_FROM_TAG
    tag  libproj_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtklibproj/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "MIT"
    SPDX_COPYRIGHT_TEXT
      "Copyright Frank Warmerdam"
    SPDX_DOWNLOAD_LOCATION
      "git+${libproj_repo}@${libproj_tag}"
    VERSION
      "${libproj_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LibPROJ
    TARGETS LibPROJ::LibPROJ
    USE_VARIABLES LibPROJ_MAJOR_VERSION
    STANDARD_INCLUDE_DIRS)

if (NOT VTK_MODULE_USE_EXTERNAL_VTK_libproj)
  set(LibPROJ_MAJOR_VERSION "8")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_libproj.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_libproj.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_libproj.h")
