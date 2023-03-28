/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebGPUActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkWebGPUActor.h"
#include "vtkMapper.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkWebGPUInternalsBindGroup.h"
#include "vtkWebGPUInternalsBindGroupLayout.h"
#include "vtkWebGPUInternalsBuffer.h"
#include "vtkWebGPUPolyDataMapper.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindow.h"

#include <algorithm>
#include <cstddef>     // for offsetof
#include <type_traits> // for underlying_type

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUActor);

//------------------------------------------------------------------------------
vtkWebGPUActor::vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
vtkWebGPUActor::~vtkWebGPUActor() = default;

//------------------------------------------------------------------------------
void vtkWebGPUActor::PrintSelf(ostream& os, vtkIndent indent) {}

//------------------------------------------------------------------------------
int vtkWebGPUActor::Update(vtkRenderer* ren, vtkMapper* mapper)
{
  auto wgpuRenWin = reinterpret_cast<vtkWebGPURenderWindow*>(ren->GetRenderWindow());
  const char* label = "ActorBuffer";
  wgpu::Device device = wgpuRenWin->GetDevice();
  if (this->ActorBuffer.Get() == nullptr)
  {
    // Create a blank buffer.
    ActorBlock actorBlock;
    this->ActorBuffer = vtkWebGPUInternalsBuffer::Upload(
      device, 0, &actorBlock, sizeof(actorBlock), wgpu::BufferUsage::Uniform, label);
    this->ActorBindGroupLayout = vtkWebGPUInternalsBindGroupLayout::MakeBindGroupLayout(device,
      {
        // clang-format off
        // actor
        { 0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform },
        // clang-format on
      });
    this->ActorBindGroupLayout.SetLabel("ActorBindGroupLayout");
    this->ActorBindGroup =
      vtkWebGPUInternalsBindGroup::MakeBindGroup(device, this->ActorBindGroupLayout,
        {
          // clang-format off
          { 0, this->ActorBuffer, 0}
          // clang-format on
        });
    this->ActorBindGroup.SetLabel("ActorBindGroup");
  }
  this->UploadActorTransforms(device);
  this->UploadActorRenderOptions(device);
  this->UploadActorShadeOptions(device);

  // Enter the UpdateBuffers mapper render type.
  // WebGPU Mappers are required to query the current render type and
  // take necessary action.
  this->CurrentMapperRenderType = MapperRenderType::UpdateBuffers;
  mapper->Render(ren, this);
  this->CurrentMapperRenderType = MapperRenderType::None;
  return 1;
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::Render(vtkRenderer* ren, vtkMapper* mapper)
{
  auto passEncoder = reinterpret_cast<vtkWebGPURenderer*>(ren)->GetRenderPassEncoder();
  passEncoder.PushDebugGroup("vtkWebGPUActor::Render");
  passEncoder.SetBindGroup(1, this->ActorBindGroup);
  this->CurrentMapperRenderType = MapperRenderType::RenderPassEncode;
  mapper->Render(ren, this);
  passEncoder.PopDebugGroup();
  this->CurrentMapperRenderType = MapperRenderType::None;
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
void vtkWebGPUActor::SetShadingType(ShadingTypeEnum shadeType, const wgpu::Device& device)
{
  // upload
  device.GetQueue().WriteBuffer(this->ActorBuffer,
    offsetof(ActorBlock, ShadeOpts) + offsetof(ActorBlock::ShadeOptions, ShadingType), &shadeType,
    sizeof(shadeType));
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::SetDirectionalMaskType(
  vtkTypeUInt32 directionalMask, const wgpu::Device& device)
{
  device.GetQueue().WriteBuffer(this->ActorBuffer,
    offsetof(ActorBlock, ShadeOpts) + offsetof(ActorBlock::ShadeOptions, DirectionalMaskType),
    &directionalMask, sizeof(directionalMask));
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::UploadActorTransforms(const wgpu::Device& device)
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
    const char* label = "ActorBuffer.Transform";
    device.GetQueue().WriteBuffer(
      this->ActorBuffer, 0, &transform, sizeof(ActorBlock::TransformInfo));
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::UploadActorRenderOptions(const wgpu::Device& device)
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
    const char* label = "ActorBuffer.RenderOpts";
    device.GetQueue().WriteBuffer(
      this->ActorBuffer, offsetof(ActorBlock, RenderOpts), &ro, sizeof(ro));
    this->RenderOptionsBuildTimestamp.Modified();
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUActor::UploadActorShadeOptions(const wgpu::Device& device)
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
    // mapper will update this when it finds out whether cell colors/point colors are available
    so.ShadingType = 0;
    // mapper will update this when it finds out what kind (point or cell) of normals are available.
    so.DirectionalMaskType = 0;
    for (int i = 0; i < 3; ++i)
    {
      so.AmbientColor[i] = this->GetProperty()->GetAmbientColor()[i];
      so.DiffuseColor[i] = this->GetProperty()->GetDiffuseColor()[i];
      so.SpecularColor[i] = this->GetProperty()->GetSpecularColor()[i];
      so.EdgeColor[i] = this->GetProperty()->GetEdgeColor()[i];
    }
    const char* label = "ActorBuffer.ShadeOpts";
    device.GetQueue().WriteBuffer(
      this->ActorBuffer, offsetof(ActorBlock, ShadeOpts), &so, sizeof(so));
    this->ShadingOptionsBuildTimestamp.Modified();
  }
}
VTK_ABI_NAMESPACE_END
