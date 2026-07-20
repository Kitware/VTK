// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWebGPUVolumeRayCastMapperInternals
 * @brief   Device side state of vtkWebGPUVolumeRayCastMapper
 *
 * Everything that names a WebGPU type lives here so that the public header of the mapper stays
 * free of webgpu_cpp.h.
 */

#ifndef vtkWebGPUVolumeRayCastMapperInternals_h
#define vtkWebGPUVolumeRayCastMapperInternals_h

#include "vtkWebGPUVolumeRayCastMapper.h"

#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"
#include "vtk_wgpu.h"

#include <map>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

class vtkImageData;
class vtkRenderer;
class vtkVolume;
class vtkVolumeProperty;
class vtkWebGPURenderWindow;

class vtkWebGPUVolumeRayCastMapper::vtkInternals
{
public:
  explicit vtkInternals(vtkWebGPUVolumeRayCastMapper* self)
    : Self(self)
  {
  }

  void ReleaseGraphicsResources();

  vtkWebGPUVolumeRayCastMapper* Self = nullptr;

  std::map<int, vtkSmartPointer<vtkVolumeProperty>> PortProperties;
};

VTK_ABI_NAMESPACE_END
#endif
// VTK-HeaderTest-Exclude: vtkWebGPUVolumeRayCastMapperInternals.h
