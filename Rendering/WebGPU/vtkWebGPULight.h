/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPULight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWebGPULight
 * @brief   OpenGL light
 *
 * vtkWebGPULight is a concrete implementation of the abstract class vtkLight.
 * vtkWebGPULight interfaces to the OpenGL rendering library.
 */

#ifndef vtkWebGPULight_h
#define vtkWebGPULight_h

#include "vtkLight.h"

#include "vtkRenderingWebGPUModule.h" // For export macro
#include "vtkType.h"                  // for types
#include "vtk_wgpu.h"                 // for webgpu

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkCamera;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPULight : public vtkLight
{
public:
  static vtkWebGPULight* New();
  vtkTypeMacro(vtkWebGPULight, vtkLight);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(vtkRenderer*, int) override{};

  struct LightInfo
  {
    // 0 : deferred, 1 : headlight, 2 : lightkit, 3 : positional
    vtkTypeUInt32 Type = 0;
    // 0 : not positional, 1 : positional
    vtkTypeUInt32 Positional = 0;
    vtkTypeFloat32 ConeAngle = 0;
    vtkTypeFloat32 Exponent = 0;
    vtkTypeFloat32 Color[3] = {};
    vtkTypeFloat32 DirectionVC[3] = {}; // normalized
    vtkTypeFloat32 PositionVC[3] = {};
    vtkTypeFloat32 Attenuation[3] = {};
  };
  LightInfo GetLightInfo(vtkRenderer* renderer, vtkCamera* camera);

protected:
  vtkWebGPULight() = default;
  ~vtkWebGPULight() override = default;

private:
  vtkWebGPULight(const vtkWebGPULight&) = delete;
  void operator=(const vtkWebGPULight&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
