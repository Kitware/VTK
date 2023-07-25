// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUActor_h
#define vtkWebGPUActor_h

#include "vtkActor.h"

#include "vtkRenderingWebGPUModule.h" // for export macro
#include "vtkTypeUInt32Array.h"       // for ivar
#include "vtk_wgpu.h"                 // for return

VTK_ABI_NAMESPACE_BEGIN
class vtkMatrix3x3;

class VTKRENDERINGWEBGPU_EXPORT vtkWebGPUActor : public vtkActor
{
public:
  static vtkWebGPUActor* New();
  vtkTypeMacro(vtkWebGPUActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void CacheActorTransforms();
  void CacheActorRenderOptions();
  void CacheActorShadeOptions();
  inline const void* GetCachedActorInformation() { return &(this->CachedActorInfo); }
  static std::size_t GetCacheSizeBytes() { return sizeof(ActorBlock); }

  /**
   * Actual actor render method.
   */
  void Render(vtkRenderer* ren, vtkMapper* mapper) override;
  wgpu::RenderBundle RenderToBundle(vtkRenderer* ren, vtkMapper* mapper);

  /**
   * Request mapper to run the vtkAlgorithm pipeline (if needed)
   * and consequently update device buffers corresponding to shader module bindings.
   * Ex: positions, colors, normals, indices
   */
  int Update(vtkRenderer* renderer, vtkMapper* mapper);

  ///@{
  /**
   * Re-use cached values in between consecutive buffer update stages.
   * Basically, never make an upstream request when our actor in the
   * MapperRenderType::RenderPassEncode stage.
   */
  vtkTypeBool HasOpaqueGeometry() override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  ///@}

  virtual bool UpdateKeyMatrices();

  // Which stage is the mapper render being called from?
  // vtkWebGPUActor::Update vs vtkWebGPUActor::Render
  enum class MapperRenderType
  {
    None,
    UpdateBuffers,
    RenderPassEncode,
    RenderBundleEncode
  };

  // mapper figures this out when updating mesh geometry.
  // if there are point scalars and we're coloring by point scalars mapped colors,
  // this variable is assigned a value of ShadingTypeEnum::Smooth.
  // if there are cell scalars and we're coloring by cell scalar mapped colors,
  // this variable is assigned a value of ShadingTypeEnum::Flat.
  enum ShadingTypeEnum : vtkTypeUInt32
  {
    Global = 0,
    Smooth,
    Flat
  };

  // What directional vectors are available to use for lighting?
  // mapper figures this out when updating mesh geometry. mappers should report
  // whether a combination of these are available by bitwise or'ing the flags.
  enum DirectionalMaskEnum : vtkTypeUInt32
  {
    NoNormals = 1 << 0,
    PointNormals = 1 << 1,
    PointTangents = 1 << 2,
    CellNormals = 1 << 3
  };

  void SetShadingType(ShadingTypeEnum shadeType);
  void SetDirectionalMaskType(vtkTypeUInt32 directionalMask);
  inline void SetMapperRenderPipelineOutdated(bool value)
  {
    this->MapperRenderPipelineOutdated = value;
  }

  inline MapperRenderType GetMapperRenderType() { return this->CurrentMapperRenderType; }
  inline wgpu::RenderBundleEncoder GetRenderBundleEncoder() { return this->CurrentBundler; }
  inline void SetDynamicOffsets(vtkSmartPointer<vtkTypeUInt32Array> offsets)
  {
    this->DynamicOffsets = offsets;
  }

protected:
  vtkWebGPUActor();
  ~vtkWebGPUActor() override;

  bool CachedMapperHasOpaqueGeometry = false;
  bool CachedMapperHasTranslucentPolygonalGeometry = false;

  MapperRenderType CurrentMapperRenderType = MapperRenderType::None;

  struct ActorBlock
  {
    struct TransformInfo
    {
      vtkTypeFloat32 World[4][4] = {};
      vtkTypeFloat32 Normal[3][4] = {};
    } Transform;

    struct RenderOptions
    {
      // Representaion - 0 : VTK_POINTS, 1 : VTK_WIREFRAME, 2 : VTK_SURFACE
      vtkTypeUInt32 Representation = 0;
      // Point size in pixels - applicable when points are visible.
      vtkTypeFloat32 PointSize = 0;
      // Line width in pixels - applicable when edges are visible.
      vtkTypeFloat32 LineWidth = 0;
      // Edge visibility - applicable for Representation = VTK_SURFACE.
      vtkTypeUInt32 EdgeVisibility = 0;
    } RenderOpts;

    struct ShadeOptions
    {
      // Material ambient color intensity.
      vtkTypeFloat32 AmbientIntensity = 0;
      // Material diffuse color intensity.
      vtkTypeFloat32 DiffuseIntensity = 0;
      // Material specular color intensity.
      vtkTypeFloat32 SpecularIntensity = 0;
      // Material specular power.
      vtkTypeFloat32 SpecularPower = 0;
      // Opacity level
      vtkTypeFloat32 Opacity = 0;
      // Shading type
      // 0: Global shading - Use global color for all primitives.
      // 1: Smooth shading - Use point based colors which will be smoothly interpolated for
      // in-between fragments. 2: Flat shading - Use cell based colors Material ambient color
      vtkTypeUInt32 ShadingType = 0;
      // What kind of normals to use for lighting? 0 - No normals, 1 - point normals, 1 - cell
      // normals
      vtkTypeUInt32 DirectionalMaskType = 0;
      // so that AmbientColor starts at 16-byte boundary.
      vtkTypeUInt8 Pad1[4] = {};
      // Material ambient color - applicable when shading type is global.
      vtkTypeFloat32 AmbientColor[4] = {};
      // Material diffuse color - applicable when shading type is global.
      vtkTypeFloat32 DiffuseColor[4] = {};
      // Material specular color - applicable when shading type is global.
      vtkTypeFloat32 SpecularColor[4] = {};
      // Edge color
      vtkTypeFloat32 EdgeColor[4] = {};
      // use this padding to make wgsl spec validator happy if needed or atleast 32 bytes.
    } ShadeOpts;
  };

  ActorBlock CachedActorInfo;

  vtkNew<vtkMatrix4x4> MCWCMatrix;
  vtkNew<vtkMatrix3x3> NormalMatrix;
  vtkNew<vtkTransform> NormalTransform;

  vtkTimeStamp ModelTransformsBuildTimestamp;
  vtkTimeStamp ShadingOptionsBuildTimestamp;
  vtkTimeStamp RenderOptionsBuildTimestamp;

  bool MapperRenderPipelineOutdated = false;
  wgpu::RenderBundleEncoder CurrentBundler;
  vtkSmartPointer<vtkTypeUInt32Array> DynamicOffsets;

private:
  vtkWebGPUActor(const vtkWebGPUActor&) = delete;
  void operator=(const vtkWebGPUActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
