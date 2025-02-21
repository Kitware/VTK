// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUActorInternals_h
#define vtkWebGPUActorInternals_h

#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtkTransform.h"
#include "vtkWebGPUPolyDataMapper.h"

#include <vtk_wgpu.h>

VTK_ABI_NAMESPACE_BEGIN

class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUActorInternals
{
  friend class vtkWebGPUPolyDataMapper;

public:
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

    struct ColorOptions
    {
      // Material ambient color - applicable when shading type is global.
      vtkTypeFloat32 AmbientColor[3] = {};
      vtkTypeUInt32 Pad1 = 0;
      // Material diffuse color - applicable when shading type is global.
      vtkTypeFloat32 DiffuseColor[3] = {};
      vtkTypeUInt32 Pad2 = 0;
      // Material specular color - applicable when shading type is global.
      vtkTypeFloat32 SpecularColor[3] = {};
      vtkTypeUInt32 Pad3 = 0;
      // Edge color
      vtkTypeFloat32 EdgeColor[3] = {};
      vtkTypeUInt32 Pad4 = 0;
      // Vertex color
      vtkTypeFloat32 VertexColor[3] = {};
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
      // Interpolation type
      vtkTypeUInt32 InterpolationType = VTK_FLAT;
      // Id to color by
      vtkTypeUInt32 Id = 0;
    } ColorOpts;
  };

  class MapperBooleanCache
  {
    bool Value = false;
    vtkTimeStamp TimeStamp;

  public:
    /**
     * Update the cached value with the new value. This also increments the TimeStamp.
     */
    inline void SetValue(bool newValue)
    {
      this->Value = newValue;
      this->TimeStamp.Modified();
    }

    /**
     * Returns the cached `Value`.
     */
    inline bool GetValue() const { return Value; }

    /**
     * Returns true if the timestamp of the cached value is older than the mapper's MTime.
     */
    inline bool IsOutdated(vtkMapper* mapper) const { return mapper->GetMTime() > this->TimeStamp; }
  };

  ActorBlock CachedActorInfo;

  MapperBooleanCache MapperHasOpaqueGeometry;
  MapperBooleanCache MapperHasTranslucentPolygonalGeometry;

  vtkNew<vtkMatrix4x4> MCWCMatrix;
  vtkNew<vtkMatrix3x3> NormalMatrix;
  vtkNew<vtkTransform> NormalTransform;

  vtkTimeStamp ModelTransformsBuildTimestamp;
  vtkTimeStamp ShadingOptionsBuildTimestamp;
  vtkTimeStamp RenderOptionsBuildTimestamp;

  wgpu::BindGroupLayout ActorBindGroupLayout;
  wgpu::BindGroup ActorBindGroup;
  wgpu::Buffer ActorBuffer;

  vtkTypeUInt32 Id = 0;

  inline void PopulateBindgroupLayouts(std::vector<wgpu::BindGroupLayout>& layouts)
  {
    layouts.emplace_back(this->ActorBindGroupLayout);
  }
};

VTK_ABI_NAMESPACE_END
#endif
