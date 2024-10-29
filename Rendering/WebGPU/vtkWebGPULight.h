// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkCamera;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPULight : public vtkLight
{
public:
  static vtkWebGPULight* New();
  vtkTypeMacro(vtkWebGPULight, vtkLight);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CacheLightInformation(vtkRenderer* renderer, vtkCamera* camera);
  inline const void* GetCachedLightInformation() { return &(this->CachedLightInfo); }
  static std::size_t GetCacheSizeBytes() { return sizeof(LightInfo); }

  void Render(vtkRenderer*, int) override;

protected:
  vtkWebGPULight() = default;
  ~vtkWebGPULight() override = default;

  struct LightInfo
  {
    vtkTypeUInt8 Pad[12] = {}; // so that Type begins at n modulo 16 byte. LightCount,
                               // a 4-byte integer is the first element in lights ssbo.
    // 0 : deferred, 1 : headlight, 2 : lightkit, 3 : positional
    vtkTypeUInt32 Type = 0;
    // 0 : not positional, 1 : positional
    vtkTypeUInt32 Positional = 0;
    vtkTypeFloat32 ConeAngle = 0;
    vtkTypeFloat32 Exponent = 0;
    vtkTypeFloat32 Color[4] = {};
    vtkTypeFloat32 DirectionVC[4] = {}; // normalized
    vtkTypeFloat32 PositionVC[4] = {};
    vtkTypeFloat32 Attenuation[4] = {};
  };
  LightInfo CachedLightInfo;

private:
  vtkWebGPULight(const vtkWebGPULight&) = delete;
  void operator=(const vtkWebGPULight&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
