/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLContextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLContextActor.h"

#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkOpenGLContextDevice3D.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkOpenGLContextActor);

//----------------------------------------------------------------------------
vtkOpenGLContextActor::vtkOpenGLContextActor()
{
}

//----------------------------------------------------------------------------
vtkOpenGLContextActor::~vtkOpenGLContextActor()
{
}

//----------------------------------------------------------------------------
void vtkOpenGLContextActor::ReleaseGraphicsResources(vtkWindow *window)
{
  vtkOpenGLContextDevice2D *device =
      vtkOpenGLContextDevice2D::SafeDownCast(this->Context->GetDevice());
  if (device)
  {
    device->ReleaseGraphicsResources(window);
  }

  if(this->Scene.GetPointer())
  {
    this->Scene->ReleaseGraphicsResources();
  }
}

//----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int vtkOpenGLContextActor::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkContextActor::RenderOverlay");

  if (!this->Context.GetPointer())
  {
    vtkErrorMacro(<< "vtkContextActor::Render - No painter set");
    return 0;
  }

  if (!this->Initialized)
  {
    this->Initialize(viewport);
  }

  vtkOpenGLContextDevice3D::SafeDownCast(
    this->Context3D->GetDevice())->Begin(viewport);

  return this->Superclass::RenderOverlay(viewport);
}


//----------------------------------------------------------------------------
void vtkOpenGLContextActor::Initialize(vtkViewport* viewport)
{
  vtkContextDevice2D *dev2D = NULL;
  vtkDebugMacro("Using OpenGL 2 for 2D rendering.")
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

    vtkOpenGLContextDevice2D *oglDev2D =
        vtkOpenGLContextDevice2D::SafeDownCast(dev2D);
    if (oglDev2D)
    {
      vtkOpenGLContextDevice3D *dev3D = vtkOpenGLContextDevice3D::New();
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
    vtkErrorMacro("Error: failed to initialize the render device.")
  }
}

//----------------------------------------------------------------------------
void vtkOpenGLContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
