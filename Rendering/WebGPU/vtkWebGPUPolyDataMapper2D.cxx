// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkViewport.h"
#include "vtkWebGPURenderer.h"

#include "Private/vtkWebGPUPolyDataMapper2DInternals.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkWebGPUPolyDataMapper2D);

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper2D::vtkWebGPUPolyDataMapper2D()
  : Internals(new vtkWebGPUPolyDataMapper2DInternals())
{
}

//------------------------------------------------------------------------------
vtkWebGPUPolyDataMapper2D::~vtkWebGPUPolyDataMapper2D() = default;

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2D::RenderOverlay(vtkViewport* viewport, vtkActor2D* actor)
{
  auto& internals = (*this->Internals);
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(viewport);

  switch (wgpuRenderer->GetRenderStage())
  {
    case vtkWebGPURenderer::RenderStageEnum::UpdatingBuffers:
      internals.UpdateBuffers(viewport, actor, this);
      break;
    case vtkWebGPURenderer::RenderStageEnum::RecordingCommands:
      if (wgpuRenderer->GetUseRenderBundles())
      {
        if (wgpuRenderer->GetRebuildRenderBundle())
        {
          internals.RecordDrawCommands(viewport, wgpuRenderer->GetRenderBundleEncoder());
        }
      }
      else
      {
        internals.RecordDrawCommands(viewport, wgpuRenderer->GetRenderPassEncoder());
      }
      break;
    default:
      break;
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUPolyDataMapper2D::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Internals->ReleaseGraphicsResources(w);
  this->Internals.reset(new vtkWebGPUPolyDataMapper2DInternals());
  this->Superclass::ReleaseGraphicsResources(w);
}

VTK_ABI_NAMESPACE_END
