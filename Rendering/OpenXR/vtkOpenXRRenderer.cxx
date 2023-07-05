// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2015, Valve Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenXRRenderer.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOpenXRCamera.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenXRRenderer);

//------------------------------------------------------------------------------
vtkOpenXRRenderer::vtkOpenXRRenderer()
{
  // better default
  this->ClippingRangeExpansion = 0.05;
}

//------------------------------------------------------------------------------
vtkCamera* vtkOpenXRRenderer::MakeCamera()
{
  vtkCamera* cam = vtkOpenXRCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}

//------------------------------------------------------------------------------
void vtkOpenXRRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
