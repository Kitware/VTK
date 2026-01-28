vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo eigen_repo
    tag  eigen_tag
  VERSION_FROM_TAG
    tag  eigen_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkeigen/COPYING.BSD"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2011, Intel Corporation"
    SPDX_DOWNLOAD_LOCATION
      "git+${eigen_repo}@${eigen_tag}"
    VERSION
      "${eigen_version}"
    HEADER_ONLY
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE Eigen3
    TARGETS Eigen3::Eigen3
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_eigen.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_eigen.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_eigen.h")
