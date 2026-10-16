# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
vtk_add_test_cxx(vtkRenderingSlangCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  TestSlangShaderManager.cxx)

vtk_test_cxx_executable(vtkRenderingSlangCxxTests tests)
