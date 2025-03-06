// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUComputePointCloudMapper.h"
#include "PointCloudMapperCopyDepthFromWindow.h"
#include "PointCloudMapperShader.h"
#include "Private/vtkWebGPUPointCloudMapperInternals.h"
#include "vtkRenderer.h"
#include "vtkWebGPUActor.h"
#include "vtkWebGPURenderWindow.h"
#include "vtkWebGPURenderer.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkWebGPUComputePointCloudMapper);

//------------------------------------------------------------------------------
vtkWebGPUComputePointCloudMapper::vtkWebGPUComputePointCloudMapper()
{
  this->Internals = vtkSmartPointer<vtkWebGPUPointCloudMapperInternals>::New();
  this->Internals->SetMapper(this);
}

//------------------------------------------------------------------------------
vtkWebGPUComputePointCloudMapper::~vtkWebGPUComputePointCloudMapper() = default;

//------------------------------------------------------------------------------
void vtkWebGPUComputePointCloudMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent.GetNextIndent());

  this->Internals->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePointCloudMapper::RenderPiece(vtkRenderer* ren, vtkActor* act)
{
  this->Internals->Initialize(ren);
  this->Internals->Update(ren);

  // Updating the camera matrix because we cannot know which renderer (and thus which camera)
  // RenderPiece was called with
  this->Internals->UploadCameraVPMatrix(ren);

  auto wgpuRenWin = vtkWebGPURenderWindow::SafeDownCast(ren->GetRenderWindow());
  if (wgpuRenWin->CheckAbortStatus())
  {
    return;
  }

  if (this->Internals->CachedInput == nullptr)
  {
    if (!this->Static)
    {
      this->GetInputAlgorithm()->Update();
    }
    this->Internals->CachedInput = this->GetInput();
  }

  const auto device = wgpuRenWin->GetDevice();
  auto wgpuActor = reinterpret_cast<vtkWebGPUActor*>(act);

  vtkWebGPUActor::MapperRenderType renderType = wgpuActor->GetMapperRenderType();
  switch (renderType)
  {
    case vtkWebGPUActor::MapperRenderType::UpdateBuffers:
    {
      this->Internals->UploadPointsToGPU();
      this->Internals->UploadColorsToGPU();

      break;
    }

    case vtkWebGPUActor::MapperRenderType::RenderPassEncode:
      VTK_FALLTHROUGH;

    case vtkWebGPUActor::MapperRenderType::RenderBundleEncode:
    {
      vtkWebGPURenderer* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(ren);
      if (wgpuRenderer == nullptr)
      {
        vtkLog(ERROR,
          "The renderer passed in RenderPiece of vtkWebGPUComputePointCloudMapper is not a WebGPU "
          "Renderer.");

        return;
      }

      wgpuRenderer->AddPostRasterizationActor(act);

      break;
    }

    case vtkWebGPUActor::MapperRenderType::RenderPostRasterization:
    {
      this->Internals->ComputePipeline->DispatchAllPasses();
      this->Internals->ComputePipeline->Update();

      this->Internals->UpdateRenderWindowDepthBuffer(ren);

      break;
    }

    case vtkWebGPUActor::MapperRenderType::None:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUComputePointCloudMapper::ComputeBounds()
{
  // Caching the input so that it can be reused by the function that uploads
  this->Internals->CachedInput = this->GetInput();

  this->InvokeEvent(vtkCommand::StartEvent, nullptr);
  if (!this->Static)
  {
    this->GetInputAlgorithm()->Update();
  }
  this->InvokeEvent(vtkCommand::EndEvent, nullptr);
  if (!this->Internals->CachedInput)
  {
    vtkMath::UninitializeBounds(this->Bounds);
    return;
  }

  // Only considering the bounds of the points, not the cells
  this->Internals->CachedInput->GetPoints()->GetBounds(this->Bounds);
}
