set(KISSFFT_SCALAR_TYPE "double")

vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo kissfft_repo
    tag  kissfft_tag)

vtk_module_third_party_internal(
  LICENSE_FILES           "vtkkissfft/COPYING"
  SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT     "Copyright (c) 2003-2010 Mark Borgerding"
  SPDX_DOWNLOAD_LOCATION  "git+${kissfft_repo}@${kissfft_tag}"
  VERSION                 "master"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_kissfft.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_kissfft.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_kissfft.h")
