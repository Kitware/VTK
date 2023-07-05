// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUActor.h"

#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindow.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUActor);

//------------------------------------------------------------------------------
vtkWebGPUActor::vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
vtkWebGPUActor::~vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
void vtkWebGPUActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkWebGPUActor::Update(vtkRenderer* ren, vtkMapper* mapper)
{
  // Enter the UpdateBuffers mapper render type.
  // WebGPU Mappers are required to query the current render type and
  // take necessary action.
  this->CurrentMapperRenderType = MapperRenderType::UpdateBuffers;
  mapper->Render(ren, this);
  this->CurrentMapperRenderType = MapperRenderType::None;
  return this->MapperRenderPipelineOutdated;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::Render(vtkRenderer* ren, vtkMapper* mapper)
{
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(ren);
  auto passEncoder = wgpuRenderer->GetRenderPassEncoder();
  auto bindGroup = wgpuRenderer->GetActorBindGroup();

  passEncoder.SetBindGroup(1, bindGroup,
    /*dynamicOffsetCount=*/this->DynamicOffsets->GetNumberOfValues(),
    /*dynamicOffsets=*/this->DynamicOffsets->GetPointer(0));

#ifndef NDEBUG
  passEncoder.PushDebugGroup("vtkWebGPUActor::Render");
#endif
  this->CurrentMapperRenderType = MapperRenderType::RenderPassEncode;
  mapper->Render(ren, this);
#ifndef NDEBUG
  passEncoder.PopDebugGroup();
#endif
  this->CurrentMapperRenderType = MapperRenderType::None;
}

//------------------------------------------------------------------------------
wgpu::RenderBundle vtkWebGPUActor::RenderToBundle(vtkRenderer* ren, vtkMapper* mapper)
{
  auto wgpuRenderer = reinterpret_cast<vtkWebGPURenderer*>(ren);
  auto wgpuRenWin = reinterpret_cast<vtkWebGPURenderWindow*>(wgpuRenderer->GetRenderWindow());
  auto sceneBindGroup = wgpuRenderer->GetSceneBindGroup();
  auto actorBindGroup = wgpuRenderer->GetActorBindGroup();

  {
    const auto colorFormat = wgpuRenWin->GetPreferredSwapChainTextureFormat();
    const int sampleCount = wgpuRenWin->GetMultiSamples() ? wgpuRenWin->GetMultiSamples() : 1;

    wgpu::RenderBundleEncoderDescriptor bundleEncDesc;
    bundleEncDesc.colorFormatsCount = 1;
    bundleEncDesc.colorFormats = &colorFormat;
    bundleEncDesc.depthStencilFormat = wgpuRenWin->GetDepthStencilFormat();
    bundleEncDesc.sampleCount = sampleCount;
    bundleEncDesc.depthReadOnly = false;
    bundleEncDesc.stencilReadOnly = false;
    bundleEncDesc.label = "Render bundle for vtkWebGPUActor";
    bundleEncDesc.nextInChain = nullptr;
    this->CurrentBundler = wgpuRenWin->NewRenderBundleEncoder(bundleEncDesc);
  }

  this->CurrentBundler.SetBindGroup(0, sceneBindGroup);
  this->CurrentBundler.SetBindGroup(1, actorBindGroup,
    /*dynamicOffsetCount=*/this->DynamicOffsets->GetNumberOfValues(),
    /*dynamicOffsets=*/this->DynamicOffsets->GetPointer(0));
#ifndef NDEBUG
  this->CurrentBundler.PushDebugGroup("vtkWebGPUActor::Render");
#endif
  this->CurrentMapperRenderType = MapperRenderType::RenderBundleEncode;
  mapper->Render(ren, this);
#ifndef NDEBUG
  this->CurrentBundler.PopDebugGroup();
#endif
  this->CurrentMapperRenderType = MapperRenderType::None;

  auto bundle = this->CurrentBundler.Finish();
  this->CurrentBundler = nullptr;
  return bundle;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkWebGPUActor::HasOpaqueGeometry()
{
  bool is_opaque = false;
  switch (this->CurrentMapperRenderType)
  {
    case MapperRenderType::None:
      break;
    case MapperRenderType::RenderPassEncode:
    { // nullify mapper so that superclass doesn't run the code path in
      // vtkMapper::HasOpaqueGeometry.
      vtkMapper* tmpMapper = this->Mapper;
      this->Mapper = nullptr;
      is_opaque = this->Superclass::HasOpaqueGeometry();
      // restore
      this->Mapper = tmpMapper;
      is_opaque &= this->CachedMapperHasOpaqueGeometry;
      break;
    }
    case MapperRenderType::UpdateBuffers:
    default:
      is_opaque = this->Superclass::HasOpaqueGeometry();
      break;
  }
  return is_opaque;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkWebGPUActor::HasTranslucentPolygonalGeometry()
{
  bool is_opaque = false;
  switch (this->CurrentMapperRenderType)
  {
    case MapperRenderType::None:
      break;
    case MapperRenderType::RenderPassEncode:
    { // nullify mapper so that superclass doesn't run the code path in
      // vtkMapper::HasTranslucentPolygonalGeometry.
      vtkMapper* tmpMapper = this->Mapper;
      this->Mapper = nullptr;
      is_opaque = this->Superclass::HasTranslucentPolygonalGeometry();
      // restore
      this->Mapper = tmpMapper;
      is_opaque &= this->CachedMapperHasTranslucentPolygonalGeometry;
      break;
    }
    case MapperRenderType::UpdateBuffers:
    default:
      is_opaque = this->Superclass::HasTranslucentPolygonalGeometry();
      break;
  }
  return is_opaque;
}

//------------------------------------------------------------------------------
bool vtkWebGPUActor::UpdateKeyMatrices()
{
  vtkMTimeType rwTime = 0;
  if (this->CoordinateSystem != WORLD && this->CoordinateSystemRenderer)
  {
    rwTime = this->CoordinateSystemRenderer->GetVTKWindow()->GetMTime();
  }

  // has the actor changed or is in device coords?
  if (this->GetMTime() > this->ModelTransformsBuildTimestamp ||
    rwTime > this->ModelTransformsBuildTimestamp || this->CoordinateSystem == DEVICE)
  {
    this->GetModelToWorldMatrix(this->MCWCMatrix);

    this->MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
    {
      this->NormalMatrix->Identity();
    }
    else
    {
      this->NormalTransform->SetMatrix(this->Matrix);
      vtkMatrix4x4* mat4 = this->NormalTransform->GetMatrix();
      for (int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, mat4->GetElement(i, j));
        }
      }
    }
    this->NormalMatrix->Invert();
    this->ModelTransformsBuildTimestamp.Modified();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::SetShadingType(ShadingTypeEnum shadeType)
{
  this->CachedActorInfo.ShadeOpts.ShadingType = shadeType;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::SetDirectionalMaskType(vtkTypeUInt32 directionalMask)
{
  this->CachedActorInfo.ShadeOpts.DirectionalMaskType = directionalMask;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CacheActorTransforms()
{
  if (this->UpdateKeyMatrices())
  {
    auto& transform = this->CachedActorInfo.Transform;
    // stage world
    std::transform(this->MCWCMatrix->GetData(), this->MCWCMatrix->GetData() + 16,
      &(transform.World[0][0]), [](double& v) -> float { return static_cast<float>(v); });
    // stage normal
    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        transform.Normal[i][j] = this->NormalMatrix->GetElement(i, j);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CacheActorRenderOptions()
{
  if (this->GetProperty()->GetMTime() > this->RenderOptionsBuildTimestamp ||
    this->GetMTime() > this->RenderOptionsBuildTimestamp)
  {
    auto& ro = this->CachedActorInfo.RenderOpts;
    const int representation = this->GetProperty()->GetRepresentation();
    ro.Representation = representation;
    ro.PointSize = this->GetProperty()->GetPointSize();
    ro.LineWidth = this->GetProperty()->GetLineWidth();
    ro.EdgeVisibility = this->GetProperty()->GetEdgeVisibility();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::CacheActorShadeOptions()
{
  if (this->GetProperty()->GetMTime() > this->ShadingOptionsBuildTimestamp ||
    this->GetMTime() > this->ShadingOptionsBuildTimestamp)
  {
    auto& so = this->CachedActorInfo.ShadeOpts;
    so.AmbientIntensity = this->GetProperty()->GetAmbient();
    so.DiffuseIntensity = this->GetProperty()->GetDiffuse();
    so.SpecularIntensity = this->GetProperty()->GetSpecular();
    so.SpecularPower = this->GetProperty()->GetSpecularPower();
    so.Opacity = this->GetProperty()->GetOpacity();
    for (int i = 0; i < 3; ++i)
    {
      so.AmbientColor[i] = this->GetProperty()->GetAmbientColor()[i];
      so.DiffuseColor[i] = this->GetProperty()->GetDiffuseColor()[i];
      so.SpecularColor[i] = this->GetProperty()->GetSpecularColor()[i];
      so.EdgeColor[i] = this->GetProperty()->GetEdgeColor()[i];
    }
  }
}
VTK_ABI_NAMESPACE_END
