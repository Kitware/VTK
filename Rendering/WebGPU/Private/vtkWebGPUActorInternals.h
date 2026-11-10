// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkWebGPUActorInternals_h
#define vtkWebGPUActorInternals_h

#include "Private/vtkWebGPUHandle.h"
#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderingWebGPUModule.h"
#include "vtkTransform.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPUTexture.h"

#include "vtk_wgpu.h"

#include <array>   // for std::array
#include <cstddef> // for offsetof

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
      // Additional custom flags used to encode various integer/boolean properties.
      vtkTypeUInt32 Flags2 = 0;
      // Coincident topology polygon offset factor (for surface/triangle pipelines).
      vtkTypeFloat32 CoinPolygonFactor = 0;
      // Coincident topology polygon offset units (for surface/triangle pipelines).
      vtkTypeFloat32 CoinPolygonOffset = 0;
      // Coincident topology line offset factor (for wireframe/line pipelines).
      vtkTypeFloat32 CoinLineFactor = 0;
      // Coincident topology line offset units (for wireframe/line pipelines).
      vtkTypeFloat32 CoinLineOffset = 0;
      // Coincident topology point offset units (for point pipelines).
      vtkTypeFloat32 CoinPointOffset = 0;
    } RenderOpts;
    vtkTypeUInt32 Pad[2];

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
      vtkTypeFloat32 Metallic = 0.0f;
      vtkTypeFloat32 Roughness = 0.5f;
      vtkTypeFloat32 BaseF0 = 0.04f;
      vtkTypeFloat32 Anisotropy = 0.0f;
      vtkTypeFloat32 AnisotropyRotation = 0.0f;
      vtkTypeFloat32 CoatStrength = 0.0f;
      vtkTypeFloat32 CoatRoughness = 0.0f;
      vtkTypeFloat32 CoatF0 = 0.0f;
      vtkTypeFloat32 CoatNormalScale = 1.0f;
      vtkTypeFloat32 NormalScale = 1.0f;
      vtkTypeFloat32 EmissiveFactor[3] = { 1.0f, 1.0f, 1.0f };
      vtkTypeFloat32 OcclusionStrength = 1.0f;
      vtkTypeFloat32 EdgeTint[3] = { 1.0f, 1.0f, 1.0f };
      vtkTypeUInt32 Pad5 = 0;
      vtkTypeFloat32 CoatColor[3] = { 1.0f, 1.0f, 1.0f };
      vtkTypeUInt32 Pad6 = 0;
      vtkTypeFloat32 BaseSpecularColor[3] = { 1.0f, 1.0f, 1.0f };
      vtkTypeFloat32 BaseSpecularWeight = 1.0f;
    } ColorOpts, BackfaceColorOpts;
  };

  static_assert(sizeof(ActorBlock::ColorOptions) == 208,
    "ColorOptions must match the layout of ActorColorOptions in ActorColorOptions.wgsl");
  static_assert(offsetof(ActorBlock, ColorOpts) == 160,
    "ColorOptions must start where the Actor struct places color_options");
  static_assert(offsetof(ActorBlock, BackfaceColorOpts) == 368,
    "BackfaceColorOptions must match the Actor struct's backface_color_options");

  static void CacheMaterial(vtkProperty* property, ActorBlock::ColorOptions& colorOptions)
  {
    colorOptions.AmbientIntensity = property->GetAmbient();
    colorOptions.DiffuseIntensity = property->GetDiffuse();
    colorOptions.SpecularIntensity = property->GetSpecular();
    colorOptions.SpecularPower = property->GetSpecularPower();
    colorOptions.Opacity = property->GetOpacity();
    for (int i = 0; i < 3; ++i)
    {
      colorOptions.AmbientColor[i] = property->GetAmbientColor()[i];
      colorOptions.DiffuseColor[i] = property->GetDiffuseColor()[i];
      colorOptions.SpecularColor[i] = property->GetSpecularColor()[i];
      colorOptions.EdgeColor[i] = property->GetEdgeColor()[i];
      colorOptions.VertexColor[i] = property->GetVertexColor()[i];
    }
    colorOptions.Metallic = static_cast<vtkTypeFloat32>(property->GetMetallic());
    colorOptions.Roughness = static_cast<vtkTypeFloat32>(property->GetRoughness());
    colorOptions.BaseF0 = static_cast<vtkTypeFloat32>(property->ComputeReflectanceOfBaseLayer());
    colorOptions.Anisotropy = static_cast<vtkTypeFloat32>(property->GetAnisotropy());
    colorOptions.AnisotropyRotation =
      static_cast<vtkTypeFloat32>(property->GetAnisotropyRotation());
    colorOptions.CoatStrength = static_cast<vtkTypeFloat32>(property->GetCoatStrength());
    colorOptions.CoatRoughness = static_cast<vtkTypeFloat32>(property->GetCoatRoughness());
    colorOptions.CoatF0 = static_cast<vtkTypeFloat32>(
      vtkProperty::ComputeReflectanceFromIOR(property->GetCoatIOR(), 1.0));
    colorOptions.CoatNormalScale = static_cast<vtkTypeFloat32>(property->GetCoatNormalScale());
    colorOptions.NormalScale = static_cast<vtkTypeFloat32>(property->GetNormalScale());
    colorOptions.OcclusionStrength = static_cast<vtkTypeFloat32>(property->GetOcclusionStrength());
    colorOptions.BaseSpecularWeight =
      static_cast<vtkTypeFloat32>(property->GetBaseSpecularWeight());
    for (int i = 0; i < 3; ++i)
    {
      colorOptions.EmissiveFactor[i] =
        static_cast<vtkTypeFloat32>(property->GetEmissiveFactor()[i]);
      colorOptions.EdgeTint[i] = static_cast<vtkTypeFloat32>(property->GetEdgeTint()[i]);
      colorOptions.CoatColor[i] = static_cast<vtkTypeFloat32>(property->GetCoatColor()[i]);
      colorOptions.BaseSpecularColor[i] =
        static_cast<vtkTypeFloat32>(property->GetBaseSpecularColor()[i]);
    }
    colorOptions.InterpolationType = static_cast<vtkTypeUInt32>(property->GetInterpolation());
  }

  class MapperBooleanCache
  {
    bool Value = false;
    vtkTimeStamp TimeStamp;

  public:
    /**
     * Update the cached value with the new value. This also increments the TimeStamp.
     */
    void SetValue(bool newValue)
    {
      this->Value = newValue;
      this->TimeStamp.Modified();
    }

    /**
     * Returns the cached `Value`.
     */
    bool GetValue() const { return Value; }

    /**
     * Returns true if the timestamp of the cached value is older than the mapper's MTime
     * or the MTime of it's input data object.
     */
    bool IsOutdated(vtkMapper* mapper) const
    {
      if (mapper->GetMTime() > this->TimeStamp)
      {
        return true;
      }
      if (auto* inputDataObject = mapper->GetInputDataObject(0, 0))
      {
        return inputDataObject->GetMTime() > this->TimeStamp;
      }
      return false;
    }
  };

  ActorBlock CachedActorInfo;

  MapperBooleanCache MapperHasOpaqueGeometry;
  MapperBooleanCache MapperHasTranslucentPolygonalGeometry;
  vtkTimeStamp MapperSwappedTime;

  vtkNew<vtkMatrix4x4> MCWCMatrix;
  vtkNew<vtkMatrix3x3> NormalMatrix;
  vtkNew<vtkTransform> NormalTransform;

  vtkTimeStamp ModelTransformsLastUpdated;
  vtkTimeStamp ShadingOptionsLastUpdated;
  vtkTimeStamp RenderOptionsLastUpdated;
  vtkTimeStamp DeviceResourcesBuildTimestamp;

  vtkWebGPU::BindGroupLayout ActorBindGroupLayout;
  vtkWebGPU::BindGroup ActorBindGroup;
  vtkWebGPU::Buffer ActorBuffer;

  vtkTypeUInt32 Id = 0;

  enum PBRTexture
  {
    PBR_ALBEDO = 0,
    PBR_MATERIAL,
    PBR_EMISSIVE,
    PBR_NORMAL,
    PBR_ANISOTROPY,
    PBR_COAT_NORMAL,
    PBR_SPECULAR,
    PBR_SPECULAR_COLOR,
    PBR_NUMBER_OF_TEXTURES
  };
  using PBRTextureArray = std::array<vtkWebGPUTexture*, PBR_NUMBER_OF_TEXTURES>;

  static PBRTextureArray GetPBRTextures(vtkActor* actor)
  {
    static constexpr const char* names[PBR_NUMBER_OF_TEXTURES] = { "albedoTex", "materialTex",
      "emissiveTex", "normalTex", "anisotropyTex", "coatNormalTex", "specularTex",
      "specularColorTex" };
    PBRTextureArray textures{};
    vtkProperty* property = actor->GetProperty();
    if (property->GetInterpolation() != VTK_PBR)
    {
      return textures;
    }
    for (int i = 0; i < PBR_NUMBER_OF_TEXTURES; ++i)
    {
      auto* texture = vtkWebGPUTexture::SafeDownCast(property->GetTexture(names[i]));
      if (texture && texture->GetDeviceResource())
      {
        textures[i] = texture;
      }
    }
    return textures;
  }

  PBRTextureArray BoundPBRTextures{};

  void PopulateBindgroupLayouts(std::vector<WGPUBindGroupLayout>& layouts)
  {
    layouts.emplace_back(this->ActorBindGroupLayout.Get());
  }
};

VTK_ABI_NAMESPACE_END
#endif
