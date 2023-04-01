/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkWebGPUCamera.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkWebGPUCamera_h
#define vtkWebGPUCamera_h

#include "vtkCamera.h"

#include "vtkMatrix4x4.h"             // for ivar
#include "vtkNew.h"                   // for ivar
#include "vtkRenderingWebGPUModule.h" // for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkMatrix3x3;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUCamera : public vtkCamera
{
public:
  static vtkWebGPUCamera* New();
  vtkTypeMacro(vtkWebGPUCamera, vtkCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CacheSceneTransforms(vtkRenderer* renderer);
  inline void* GetCachedSceneTransforms() { return &(this->CachedSceneTransforms); }
  static std::size_t GetCacheSizeBytes() { return sizeof(SceneTransforms); }

  /**
   * Implement base class method. This function does not actually 'render' anything.
   * It only brings up view and projection matrices bindgroup.
   * Unfortunately, we inherit the superclass method name because vtkRendererer::UpdateCamera calls
   * Render on it's ActiveCamera.
   */
  void Render(vtkRenderer* renderer) override;

  /**
   * Invokes setViewport, setScissor (if needed) followed by updating the view, projection buffers.
   * The WebGPU renderer will invoke this method prior to rendering it's props.
   */
  void UpdateViewport(vtkRenderer* renderer) override;

protected:
  vtkWebGPUCamera();
  ~vtkWebGPUCamera() override;

  vtkTimeStamp KeyMatrixTime;
  vtkRenderer* LastRenderer;
  vtkNew<vtkMatrix3x3> NormalMatrix;

  // Pack uniforms as needed.
  struct SceneTransforms
  {
    // Viewport dimensions
    vtkTypeFloat32 Viewport[4];
    // World -> Camera space
    vtkTypeFloat32 ViewMatrix[4][4] = {};
    // Camera space -> Clipped space
    vtkTypeFloat32 ProjectionMatrix[4][4] = {};
    // Normal matrix Inverted and transposed ViewMatrix
    vtkTypeFloat32 NormalMatrix[4][4] = {};
    // Clipped space -> Camera space
    vtkTypeFloat32 InvertedProjectionMatrix[4][4] = {};
  };
  SceneTransforms CachedSceneTransforms;

private:
  vtkWebGPUCamera(const vtkWebGPUCamera&) = delete;
  void operator=(const vtkWebGPUCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
