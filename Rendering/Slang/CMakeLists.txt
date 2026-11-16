# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause
if (VTK_USE_SLANG)
  vtk_module_find_package(PRIVATE_IF_SHARED
    PACKAGE SlangKit
    VERSION 0.1)
endif ()

set(classes
  vtkSlangShaderManager)

vtk_module_add_module(VTK::RenderingSlang
  CLASSES ${classes})

if (VTK_USE_SLANG)
  vtk_module_link(VTK::RenderingSlang
    PRIVATE
      SlangKit::SlangKit)
  vtk_module_definitions(VTK::RenderingSlang
    PRIVATE
      VTK_RENDERINGSLANG_USE_SLANGKIT)
endif ()
