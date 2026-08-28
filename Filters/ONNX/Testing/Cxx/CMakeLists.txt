# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
vtk_add_test_cxx(vtkFiltersONNXCxxTests tests
  TestFindMatchingPermutation.cxx,NO_VALID,NO_DATA
  TestONNXInference.cxx,NO_VALID
  )

vtk_test_cxx_executable(vtkFiltersONNXCxxTests tests)
