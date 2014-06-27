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
#include "vtkOpenGLContextDevice2D.h"
#include "vtkOpenGL2ContextDevice2D.h"

#include "vtkContext3D.h"
#include "vtkOpenGLContextDevice3D.h"
#include "vtkContextScene.h"
#include "vtkObjectFactory.h"

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
void vtkOpenGLContextActor::Initialize(vtkViewport* viewport)
{
  vtkOpenGLContextDevice3D *dev = vtkOpenGLContextDevice3D::New();
  this->Context3D->Begin(dev);
  dev->Delete();

  vtkContextDevice2D *device = NULL;
  if (vtkOpenGL2ContextDevice2D::IsSupported(viewport))
    {
    vtkDebugMacro("Using OpenGL 2 for 2D rendering.")
    device = vtkOpenGL2ContextDevice2D::New();
    }
  else
    {
    vtkDebugMacro("Using OpenGL 1 for 2D rendering.")
    device = vtkOpenGLContextDevice2D::New();
    }
  if (device)
    {
    this->Context->Begin(device);
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
