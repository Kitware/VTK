// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
  vtkRenderer* LastRenderer = nullptr;
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
    vtkTypeFloat32 NormalMatrix[3][4] = {};
    // Clipped space -> Camera space
    vtkTypeFloat32 InvertedProjectionMatrix[4][4] = {};
    // Custom flags used to encode various integer/boolean properties.
    vtkTypeUInt32 Flags = 0;
  };
  SceneTransforms CachedSceneTransforms;

private:
  vtkWebGPUCamera(const vtkWebGPUCamera&) = delete;
  void operator=(const vtkWebGPUCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
