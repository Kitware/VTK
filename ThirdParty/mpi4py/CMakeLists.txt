#==========================================================================
#
#     Program: ParaView
#
#     Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
#     All rights reserved.
#
#     ParaView is a free software; you can redistribute it and/or modify it
#     under the terms of the ParaView license version 1.2.
#
#     See License_v1.2.txt for the full ParaView license.
#     A copy of this license can be obtained by contacting
#     Kitware Inc.
#     28 Corporate Drive
#     Clifton Park, NY 12065
#     USA
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#==========================================================================
vtk_module_impl()
vtk_module_export("")

option(VTK_USE_SYSTEM_MPI4PY "Use system 'mpi4py' Python package" OFF)
mark_as_advanced(VTK_USE_SYSTEM_MPI4PY)

if (NOT VTK_USE_SYSTEM_MPI4PY)
  # Configure mpi4py.
  set (MPI4PY_INSTALL_PACKAGE_DIR "${VTK_INSTALL_LIBRARY_DIR}/site-packages")
  set (MPI4PY_PACKAGE_BINARY_DIR  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/site-packages")

  # Block warnings unless we are instructed to allow them.
  # SET(VTKMPI4PY_WARNINGS_ALLOW 1)
  vtk_third_party_warning_suppress(VTKMPI4PY C)

  add_subdirectory(vtkmpi4py)
endif ()
