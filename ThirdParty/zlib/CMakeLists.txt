vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo zlib_repo
    tag  zlib_tag
  VERSION_FROM_TAG
    tag  zlib_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkzlib/LICENSE"
    SPDX_LICENSE_IDENTIFIER
      "Zlib"
    SPDX_COPYRIGHT_TEXT
      "(C) 1995-2024 Jean-loup Gailly and Mark Adler"
    SPDX_DOWNLOAD_LOCATION
      "git+${zlib_repo}@${zlib_tag}"
    VERSION
      "${zlib_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE ZLIB
    TARGETS ZLIB::ZLIB
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_zlib.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_zlib.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_zlib.h")
