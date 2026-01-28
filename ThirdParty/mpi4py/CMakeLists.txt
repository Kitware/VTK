vtk_third_party_extract(
  FILE "${CMAKE_CURRENT_SOURCE_DIR}/update.sh"
  TAGS
    repo mpi4py_repo
    tag  mpi4py_tag
  VERSION_FROM_TAG
    tag  mpi4py_version "^for/vtk-[0-9]*-([0-9\\.]+)$")

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkmpi4py/LICENSE.rst"
    SPDX_LICENSE_IDENTIFIER
      "BSD-2-Clause AND BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2021, Lisandro Dalcin"
      "Copyright (c) Sandia Corporation, Kitware Inc."
    SPDX_DOWNLOAD_LOCATION
      "git+${mpi4py_repo}@${mpi4py_tag}"
    VERSION
      "${mpi4py_version}"
  EXTERNAL
    PACKAGE mpi4py
    TARGETS mpi4py::mpi4py)
