vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo lzma_repo
    tag  lzma_tag
  VERSION_FROM_TAG
    tag  lzma_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtklzma/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "LicenseRef-PublicDomain-lzma"
    SPDX_COPYRIGHT_TEXT
      "NONE"
    SPDX_DOWNLOAD_LOCATION
      "git+${lzma_repo}@${lzma_tag}"
    SPDX_CUSTOM_LICENSE_FILE
      "vtklzma/COPYING"
    SPDX_CUSTOM_LICENSE_NAME
      "PublicDomain-lzma"
    VERSION
      "${lzma_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE LZMA
    TARGETS LZMA::LZMA
    STANDARD_INCLUDE_DIRS)

include(vtkDetectLibraryType)
vtk_detect_library_shared(vtklzma LZMA::LZMA)

set(LZMA_BUILT_AS_DYNAMIC_LIB "${vtklzma_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_lzma.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_lzma.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_lzma.h")
