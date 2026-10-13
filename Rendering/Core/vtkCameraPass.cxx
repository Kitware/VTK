// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCameraPass.h"
#include "vtkObjectFactory.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include <cassert>

VTK_ABI_NAMESPACE_BEGIN
vtkObjectFactoryNewMacro(vtkCameraPass);
vtkCxxSetObjectMacro(vtkCameraPass, DelegatePass, vtkRenderPass);

//------------------------------------------------------------------------------
vtkCameraPass::vtkCameraPass()
{
  this->DelegatePass = nullptr;
  this->AspectRatioOverride = 1.0;
}

//------------------------------------------------------------------------------
vtkCameraPass::~vtkCameraPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkCameraPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AspectRatioOverride: " << this->AspectRatioOverride << endl;
  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}

void vtkCameraPass::GetTiledSizeAndOrigin(
  const vtkRenderState* render_state, int* width, int* height, int* originX, int* originY)
{
  vtkRenderer* ren = render_state->GetRenderer();
  ren->GetTiledSizeAndOrigin(width, height, originX, originY);
}

//------------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkCameraPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
VTK_ABI_NAMESPACE_END
