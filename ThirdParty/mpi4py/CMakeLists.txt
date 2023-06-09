#==========================================================================
#
#     Program: ParaView
#
# Copyright (c) Sandia Corporation, Kitware Inc.
# All rights reserved.
# See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
#
#    This software is distributed WITHOUT ANY WARRANTY; without even
#    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
#    PURPOSE.  See the above copyright notice for more information.
#
#
#==========================================================================

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtkmpi4py/LICENSE.rst"
    SPDX_LICENSE_IDENTIFIER
      "BSD-2-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright (c) 2021, Lisandro Dalcin"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/mpi4py.git@for/vtk-20230424-3.1.4"
    VERSION
      "3.1.4"
  EXTERNAL
    PACKAGE mpi4py
    TARGETS mpi4py::mpi4py)
