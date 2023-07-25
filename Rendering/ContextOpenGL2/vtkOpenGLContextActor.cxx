// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkOpenGLContextActor.h"

#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkOpenGLContextDevice3D.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkOpenGLContextActor);

//------------------------------------------------------------------------------
vtkOpenGLContextActor::vtkOpenGLContextActor() = default;

//------------------------------------------------------------------------------
vtkOpenGLContextActor::~vtkOpenGLContextActor() = default;

//------------------------------------------------------------------------------
void vtkOpenGLContextActor::ReleaseGraphicsResources(vtkWindow* window)
{
  vtkOpenGLContextDevice2D* device =
    vtkOpenGLContextDevice2D::SafeDownCast(this->Context->GetDevice());
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
// Renders an actor2D's property and then it's mapper.
int vtkOpenGLContextActor::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkContextActor::RenderOverlay");

  if (!this->Context)
  {
    vtkErrorMacro(<< "vtkContextActor::Render - No painter set");
    return 0;
  }

  if (!this->Initialized)
  {
    this->Initialize(viewport);
  }

  vtkOpenGLContextDevice3D::SafeDownCast(this->Context3D->GetDevice())->Begin(viewport);

  return this->Superclass::RenderOverlay(viewport);
}

//------------------------------------------------------------------------------
void vtkOpenGLContextActor::Initialize(vtkViewport* viewport)
{
  vtkContextDevice2D* dev2D = nullptr;
  vtkDebugMacro("Using OpenGL 2 for 2D rendering.");
  if (this->ForceDevice)
  {
    dev2D = this->ForceDevice;
    dev2D->Register(this);
  }
  else
  {
    dev2D = vtkOpenGLContextDevice2D::New();
  }

  if (dev2D)
  {
    this->Context->Begin(dev2D);

    vtkOpenGLContextDevice2D* oglDev2D = vtkOpenGLContextDevice2D::SafeDownCast(dev2D);
    if (oglDev2D)
    {
      vtkOpenGLContextDevice3D* dev3D = vtkOpenGLContextDevice3D::New();
      dev3D->Initialize(vtkRenderer::SafeDownCast(viewport), oglDev2D);
      this->Context3D->Begin(dev3D);
      dev3D->Delete();
    }

    dev2D->Delete();
    this->Initialized = true;
  }
  else
  {
    // Failed
    vtkErrorMacro("Error: failed to initialize the render device.");
  }
}

//------------------------------------------------------------------------------
void vtkOpenGLContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
