// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2015, Valve Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenVRRenderer.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkOpenVRCamera.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenVRRenderer);

//------------------------------------------------------------------------------
vtkCamera* vtkOpenVRRenderer::MakeCamera()
{
  vtkCamera* cam = vtkOpenVRCamera::New();
  this->InvokeEvent(vtkCommand::CreateCameraEvent, cam);
  return cam;
}
VTK_ABI_NAMESPACE_END
