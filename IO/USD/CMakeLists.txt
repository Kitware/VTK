# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause

vtk_module_find_package(PRIVATE_IF_SHARED
  PACKAGE pxr)

set(classes
  vtkUSDExporter)

vtk_module_add_module(VTK::IOUSD
  CLASSES ${classes})

vtk_module_link(VTK::IOUSD
  NO_KIT_EXPORT_IF_SHARED
  PRIVATE ${PXR_LIBRARIES})

vtk_module_include(VTK::IOUSD
  PRIVATE ${PXR_INCLUDE_DIRS})

vtk_add_test_mangling(VTK::IOUSD)
