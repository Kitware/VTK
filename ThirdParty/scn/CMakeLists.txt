vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo scn_repo
    tag  scn_tag
  VERSION_FROM_TAG
    tag  scn_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      vtkscn/LICENSE
      vtkscn/LICENSE.nanorange
    SPDX_LICENSE_IDENTIFIER "Apache-2.0 AND BSL-1.0"
    SPDX_COPYRIGHT_TEXT     "Copyright (c) 2017 Elias Kosunen; Copyright (c) 2018 Tristan Brindle (tcbrindle at gmail dot com)"
    SPDX_DOWNLOAD_LOCATION  "git+${scn_repo}@${scn_tag}"
    VERSION                 "${scn_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE scn
    TARGETS scn::scn
    VERSION       "4.0.0"
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_scn.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_scn.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_scn.h")
