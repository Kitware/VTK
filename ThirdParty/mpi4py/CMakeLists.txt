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
      "git+https://gitlab.kitware.com/third-party/mpi4py.git@for/vtk-20230424-3.1.4"
    VERSION
      "3.1.4"
  EXTERNAL
    PACKAGE mpi4py
    TARGETS mpi4py::mpi4py)
