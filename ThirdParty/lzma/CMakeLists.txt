vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtklzma/COPYING"
    VERSION       "5.2.4"
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
