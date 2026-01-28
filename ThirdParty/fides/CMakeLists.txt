vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo fides_repo
    tag  fides_tag)

vtk_module_third_party_internal(
  LICENSE_FILES           "vtkfides/LICENSE.txt"
  SPDX_LICENSE_IDENTIFIER "BSD-3-Clause"
  SPDX_COPYRIGHT_TEXT     "Copyright (c) 2019 Kitware Inc."
  SPDX_DOWNLOAD_LOCATION  "git+${fides_repo}@${fides_tag}"
  VERSION                 "master"
  SUBDIRECTORY            vtkfides
  STANDARD_INCLUDE_DIRS
  INTERFACE)
