# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
vtk_module_test_data(
  Data/ONNX/gaussian_kernel.onnx
  Data/ONNX/linear_adder.onnx)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()
