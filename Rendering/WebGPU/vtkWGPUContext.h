/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWGPUContext.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWGPUContext_h
#define vtkWGPUContext_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_EXPORT vtkWGPUContext
{
public:
  static void LogAvailableAdapters();
  static void GetAdapterInfo(char (*adapter_info)[256]);
  static wgpu::Adapter RequestAdapter(const wgpu::RequestAdapterOptions& options);
  static wgpu::Device RequestDevice(
    const wgpu::Adapter& adapter, const wgpu::DeviceDescriptor& deviceDescriptor);
  static wgpu::Surface CreateSurface(const wgpu::ChainedStruct& surfaceDescriptor);
  static std::size_t Align(std::size_t value, int alignment);
};

VTK_ABI_NAMESPACE_END

#endif // vtkWGPUContext_h
// VTK-HeaderTest-Exclude: vtkWGPUContext.h
