vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo h5hut_repo
    tag  h5hut_tag
  VERSION_FROM_TAG
    tag  h5hut_version "^for/vtk-[0-9]*-v([0-9\\.rc]+)$")

# we don't support external h5hut
vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkh5hut/COPYING"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2006-2016, The Regents of the University of California, through Lawrence Berkeley National Laboratory (subject to receipt of any"
    "required approvals from the U.S. Dept. of Energy) and the Paul Scherrer Institut (Switzerland)."
  SPDX_DOWNLOAD_LOCATION
    "git+${h5hut_repo}@${h5hut_tag}"
  VERSION
    "${h5hut_version}"
  STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_h5hut.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_h5hut.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_h5hut.h")
