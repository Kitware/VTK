# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE onnxruntime)

set(classes
  )

set(private_headers
  )

vtk_module_add_module(VTK::FiltersONNX
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})

vtk_module_link(VTK::FiltersONNX
  NO_KIT_EXPORT_IF_SHARED
  PRIVATE
    onnxruntime::onnxruntime)

vtk_add_test_mangling(VTK::FiltersONNX)
