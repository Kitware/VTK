vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo gl2ps_repo
    tag  gl2ps_tag
  VERSION_FROM_TAG
    tag  gl2ps_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES           "vtkgl2ps/COPYING.GL2PS"
    SPDX_LICENSE_IDENTIFIER "GL2PS"
    SPDX_COPYRIGHT_TEXT     "Copyright (C) 2003, Christophe Geuzaine"
    SPDX_DOWNLOAD_LOCATION  "git+${gl2ps_repo}@${gl2ps_tag}"
    VERSION                 "${gl2ps_version}"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE GL2PS
    VERSION 1.4.2
    TARGETS GL2PS::GL2PS
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_gl2ps.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_gl2ps.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_gl2ps.h")
