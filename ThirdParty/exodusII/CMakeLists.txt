vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo exodusII_repo
    tag  exodusII_tag)

vtk_module_third_party_internal(
  LICENSE_FILES
    "vtkexodusII/COPYRIGHT"
  SPDX_LICENSE_IDENTIFIER
    "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT
    "Copyright (c) 2005-2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS)"
  SPDX_DOWNLOAD_LOCATION
    "git+${exodusII_repo}@${exodusII_tag}#packages/seacas/libraries/exodus"
  VERSION
    "9.0.4"
  STANDARD_INCLUDE_DIRS)

#configure_file(
#  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exodusII.h.in"
#  "${CMAKE_CURRENT_BINARY_DIR}/vtk_exodusII.h")

vtk_module_install_headers(
#  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_exodusII.h")
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/vtk_exodusII.h")
