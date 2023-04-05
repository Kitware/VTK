/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUInternalsBuffer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWebGPUInternalsBuffer_h
#define vtkWebGPUInternalsBuffer_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUInternalsBuffer
{
public:
  static wgpu::Buffer Upload(const wgpu::Device& device, unsigned long offset, void* data,
    unsigned long sizeBytes, wgpu::BufferUsage usage, const char* label = nullptr);

  static wgpu::Buffer CreateABuffer(const wgpu::Device& device, unsigned long sizeBytes,
    wgpu::BufferUsage usage, bool mappedAtCreation = false, const char* label = nullptr);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUInternalsBuffer.h
