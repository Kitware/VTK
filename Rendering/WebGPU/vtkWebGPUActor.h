// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUActor_h
#define vtkWebGPUActor_h

#include "vtkActor.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtk_wgpu.h"                 // for return

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;
class vtkWebGPURenderPipelineCache;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUActor : public vtkActor
{
public:
  static vtkWebGPUActor* New();
  vtkTypeMacro(vtkWebGPUActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  inline const void* GetCachedActorInformation() { return &(this->CachedActorInfo); }
  static std::size_t GetCacheSizeBytes() { return sizeof(ActorBlock); }

  /**
   * Actual actor render method.
   */
  void Render(vtkRenderer* renderer, vtkMapper* mapper) override;

  /**
   * Returns true if the actor supports rendering with render bundles, false otherwise.
   *
   * This is mainly used for the point cloud mapper. This mapper doesn't use the rasterization
   * pipeline for the rendering and thus doesn't support render bundles.
   */
  bool SupportRenderBundles();

  virtual bool UpdateKeyMatrices();

  /**
   * Forces the renderer to re-record draw commands into a render bundle associated with this actor.
   *
   * @note This does not use vtkSetMacro because the actor MTime should not be affected when a
   * render bundle is invalidated.
   */
  inline void SetBundleInvalidated(bool value) { this->BundleInvalidated = value; }

  /**
   * Get whether the render bundle associated with this actor must be reset by the renderer.
   */
  vtkGetMacro(BundleInvalidated, bool);

protected:
  vtkWebGPUActor();
  ~vtkWebGPUActor() override;

  void CacheActorTransforms();
  void CacheActorRenderOptions();
  void CacheActorShadeOptions();

  struct ActorBlock
  {
    struct TransformInfo
    {
      vtkTypeFloat32 World[4][4] = {};
      vtkTypeFloat32 Normal[3][4] = {};
    } Transform;

    struct RenderOptions
    {
      // Point size in pixels - applicable when points are visible.
      vtkTypeFloat32 PointSize = 0;
      // Line width in pixels - applicable when lines/edges are visible.
      vtkTypeFloat32 LineWidth = 0;
      // Edge width in pixels - applicable when edges are visible and UseLineWidthForEdgeThickness
      // is false.
      vtkTypeFloat32 EdgeWidth = 0;
      // Custom flags used to encode various integer/boolean properties.
      vtkTypeUInt32 Flags = 0;
    } RenderOpts;

    struct ShadeOptions
    {
      // Material ambient color intensity.
      vtkTypeFloat32 AmbientIntensity = 0;
      // Material diffuse color intensity.
      vtkTypeFloat32 DiffuseIntensity = 1;
      // Material specular color intensity.
      vtkTypeFloat32 SpecularIntensity = 0;
      // Material specular power.
      vtkTypeFloat32 SpecularPower = 0;
      // Opacity level
      vtkTypeFloat32 Opacity = 0;
      // So that `AmbientColor` starts at 16-byte boundary.
      vtkTypeUInt32 Pad[3];
      // Material ambient color - applicable when shading type is global.
      vtkTypeFloat32 AmbientColor[4] = {};
      // Material diffuse color - applicable when shading type is global.
      vtkTypeFloat32 DiffuseColor[4] = {};
      // Material specular color - applicable when shading type is global.
      vtkTypeFloat32 SpecularColor[4] = {};
      // Edge color
      vtkTypeFloat32 EdgeColor[4] = {};
      // Vertex color
      vtkTypeFloat32 VertexColor[4] = {};
    } ShadeOpts;
  };

  ActorBlock CachedActorInfo;

  vtkNew<vtkMatrix4x4> MCWCMatrix;
  vtkNew<vtkMatrix3x3> NormalMatrix;
  vtkNew<vtkTransform> NormalTransform;

  vtkTimeStamp ModelTransformsBuildTimestamp;
  vtkTimeStamp ShadingOptionsBuildTimestamp;
  vtkTimeStamp RenderOptionsBuildTimestamp;

  bool BundleInvalidated = false;

private:
  vtkWebGPUActor(const vtkWebGPUActor&) = delete;
  void operator=(const vtkWebGPUActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
