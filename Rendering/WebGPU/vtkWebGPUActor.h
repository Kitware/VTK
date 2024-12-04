// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUActor_h
#define vtkWebGPUActor_h

#include "vtkActor.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtk_wgpu.h"                 // for return

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;
class vtkWebGPUConfiguration;
class vtkWebGPURenderPipelineCache;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUActor : public vtkActor
{
public:
  static vtkWebGPUActor* New();
  vtkTypeMacro(vtkWebGPUActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void ReleaseGraphicsResources(vtkWindow* window) override;

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

  inline void PopulateBindgroupLayouts(std::vector<wgpu::BindGroupLayout>& layouts)
  {
    layouts.emplace_back(this->ActorBindGroupLayout);
  }

  virtual bool UpdateKeyMatrices();

  ///@{
  /**
   * Does this prop have opaque/translucent polygonal geometry?
   * These methods are overriden to skip redundant checks
   * in different rendering stages.
   *
   * If the mapper has already been checked for opaque geometry and the mapper
   * has not been modified since the last check, this method uses the last result,
   * instead of asking the mapper to check for opaque geometry again. The
   * HasTranslucentPolygonalGeometry() similarly checks and caches the result of
   * vtkMapper::HasTranslucentPolygonalGeometry()
   *
   * @sa vtkWebGPURenderer::GetRenderStage()
   */
  vtkTypeBool HasOpaqueGeometry() override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

protected:
  vtkWebGPUActor();
  ~vtkWebGPUActor() override;

  bool CacheActorTransforms();
  bool CacheActorRenderOptions();
  bool CacheActorShadeOptions();

  void AllocateResources(vtkWebGPUConfiguration* renderer);

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

  wgpu::BindGroupLayout ActorBindGroupLayout;
  wgpu::BindGroup ActorBindGroup;
  wgpu::Buffer ActorBuffer;

  class MapperBooleanCache
  {
    bool Value = false;
    vtkTimeStamp TimeStamp;

  public:
    /**
     * Update the cached value with the new value. This also increments the TimeStamp.
     */
    void SetValue(bool newValue);

    /**
     * Returns the cached `Value`.
     */
    inline bool GetValue() { return Value; }

    /**
     * Returns true if the timestamp of the cached value is older than the mapper's MTime.
     */
    bool IsOutdated(vtkMapper* mapper);
  };

  MapperBooleanCache MapperHasOpaqueGeometry;
  MapperBooleanCache MapperHasTranslucentPolygonalGeometry;

private:
  vtkWebGPUActor(const vtkWebGPUActor&) = delete;
  void operator=(const vtkWebGPUActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
