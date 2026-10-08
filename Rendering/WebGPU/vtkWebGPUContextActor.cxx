// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWebGPUContextActor.h"

#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkWebGPUContextDevice2D.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkWebGPUContextActor);

//------------------------------------------------------------------------------
vtkWebGPUContextActor::vtkWebGPUContextActor() = default;

//------------------------------------------------------------------------------
vtkWebGPUContextActor::~vtkWebGPUContextActor() = default;

//------------------------------------------------------------------------------
void vtkWebGPUContextActor::ReleaseGraphicsResources(vtkWindow* window)
{
  vtkWebGPUContextDevice2D* device =
    vtkWebGPUContextDevice2D::SafeDownCast(this->Context->GetDevice());
  if (device)
  {
    device->ReleaseGraphicsResources(window);
  }

  if (this->Scene)
  {
    this->Scene->ReleaseGraphicsResources();
  }
}

//------------------------------------------------------------------------------
int vtkWebGPUContextActor::RenderOverlay(vtkViewport* viewport)
{
  return this->Superclass::RenderOverlay(viewport);
}

//------------------------------------------------------------------------------
void vtkWebGPUContextActor::Initialize(vtkViewport* /*viewport*/)
{
  vtkContextDevice2D* device2D = nullptr;
  vtkDebugMacro("Using WebGPU for 2D rendering.");
  if (this->ForceDevice)
  {
    device2D = this->ForceDevice;
    device2D->Register(this);
  }
  else
  {
    device2D = vtkWebGPUContextDevice2D::New();
  }

  if (device2D)
  {
    this->Context->Begin(device2D);

    device2D->Delete();
    this->Initialized = true;
  }
  else
  {
    vtkErrorMacro("Error: failed to initialize the render device.");
  }
}

//------------------------------------------------------------------------------
void vtkWebGPUContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
