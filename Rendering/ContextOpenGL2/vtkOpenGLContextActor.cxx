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
  vtkOpenGLContextDevice2D *device = NULL;
  vtkDebugMacro("Using OpenGL 2 for 2D rendering.")
  device = vtkOpenGLContextDevice2D::New();
  if (device)
  {
    this->Context->Begin(device);

    vtkOpenGLContextDevice3D *dev = vtkOpenGLContextDevice3D::New();
    dev->Initialize(vtkRenderer::SafeDownCast(viewport), device);
    this->Context3D->Begin(dev);
    dev->Delete();

    device->Delete();
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
