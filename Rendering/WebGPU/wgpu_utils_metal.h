/*=========================================================================

  Program:   Visualization Toolkit
  Module:    wgpu_utils_metal.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"
#include <memory>

static VTKRENDERINGWEBGPU_EXPORT std::unique_ptr<wgpu::ChainedStruct> VTKRENDERINGWEBGPU_EXPORT
SetupWindowAndGetSurfaceDescriptorCocoa(void* window);
