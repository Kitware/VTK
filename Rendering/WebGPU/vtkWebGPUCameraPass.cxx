// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUCameraPass.h"
#include "vtkObjectFactory.h"
#include "vtkOverrideAttribute.h"
#include "vtkRenderState.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkWebGPURenderer.h"
#include "vtkWindow.h"
#include "vtk_wgpu.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPUCameraPass);

//------------------------------------------------------------------------------
vtkWebGPUCameraPass::vtkWebGPUCameraPass() = default;

//------------------------------------------------------------------------------
vtkWebGPUCameraPass::~vtkWebGPUCameraPass() = default;

//------------------------------------------------------------------------------
vtkOverrideAttribute* vtkWebGPUCameraPass::CreateOverrideAttributes()
{
  auto* renderingBackendAttribute =
    vtkOverrideAttribute::CreateAttributeChain("RenderingBackend", "WebGPU", nullptr);
  return renderingBackendAttribute;
}

//------------------------------------------------------------------------------
void vtkWebGPUCameraPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
std::tuple<int, int, int, int> vtkWebGPUCameraPass::ComputeYInvertedViewport(vtkRenderer* renderer)
{
  int origin[2];
  int width, height;
  renderer->GetTiledSizeAndOrigin(&width, &height, &origin[0], &origin[1]);
  if (auto* window = renderer->GetVTKWindow())
  {
    // origin, width and height are in the composited window coordinate system, so use actual size
    // instead of the individual tile size.
    int* actSize = window->GetActualSize();
    origin[1] = actSize[1] - origin[1] - height;
  }
  return { origin[0], origin[1], width, height };
}

//------------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkWebGPUCameraPass::Render(const vtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;

  vtkRenderer* ren = s->GetRenderer();

  if (!ren->IsActiveCameraCreated())
  {
    vtkDebugMacro(<< "No cameras are on, creating one.");
    // the get method will automagically create a camera
    // and reset it since one hasn't been specified yet.
    // If is very unlikely that this can occur - if this
    // renderer is part of a vtkRenderWindow, the camera
    // will already have been created as part of the
    // DoStereoRender() method.

    // this is ren->GetActiveCameraAndResetIfCreated();
    ren->GetActiveCamera();
    ren->ResetCamera();
  }

  auto rpassEncoder = reinterpret_cast<vtkWebGPURenderer*>(ren)->GetRenderPassEncoder();
  auto [originX, originY, width, height] = this->ComputeYInvertedViewport(ren);
  // Set viewport frustum
  wgpuRenderPassEncoderSetViewport(rpassEncoder, originX, originY, static_cast<float>(width),
    static_cast<float>(height), 0.0, 1.0);

  if (width > 0 && height > 0)
  {
    // zero width/height case causes validation error. it can happen for certail tiled
    // configurations.
    wgpuRenderPassEncoderSetScissorRect(
      rpassEncoder, originX, originY, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
  }

  if (ren->GetRenderWindow()->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Render(s);
    this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();
  }
  else
  {
    vtkWarningMacro(<< " no delegate.");
  }
}
VTK_ABI_NAMESPACE_END
