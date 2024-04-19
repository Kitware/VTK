vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtklzma/COPYING"
    SPDX_LICENSE_IDENTIFIER
      "LicenseRef-PublicDomain-lzma"
    SPDX_COPYRIGHT_TEXT
      "NONE"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/lzma.git@for/vtk-20210728-5.2.5"
    SPDX_CUSTOM_LICENSE_FILE
      "vtklzma/COPYING"
    SPDX_CUSTOM_LICENSE_NAME
      "PublicDomain-lzma"
    VERSION
      "5.2.5"
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
