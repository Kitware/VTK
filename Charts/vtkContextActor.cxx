/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContextActor.h"

#include "vtkContext2D.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkContextScene.h"

#include "vtkViewport.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkContextActor, "1.2");
vtkStandardNewMacro(vtkContextActor);

vtkCxxSetObjectMacro(vtkContextActor, Context, vtkContext2D);
vtkCxxSetObjectMacro(vtkContextActor, Scene, vtkContextScene);

//----------------------------------------------------------------------------
// Creates an actor2D with the following defaults:
// position -1, -1 (view coordinates)
// orientation 0, scale (1,1), layer 0, visibility on
vtkContextActor::vtkContextActor()
{
  this->Context = vtkContext2D::New();
  vtkOpenGLContextDevice2D *pd = vtkOpenGLContextDevice2D::New();
  this->Context->Begin(pd);

  this->Scene = vtkContextScene::New();
}

//----------------------------------------------------------------------------
// Destroy an actor2D.
vtkContextActor::~vtkContextActor()
{
  if (this->Context)
    {
    this->Context->GetDevice()->Delete();
    this->Context->Delete();
    this->Context = NULL;
    }

  if (this->Scene)
    {
    this->Scene->Delete();
    this->Scene = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkContextActor::ReleaseGraphicsResources(vtkWindow *)
{
}

//----------------------------------------------------------------------------
// Renders an actor2D's property and then it's mapper.
int vtkContextActor::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkContextActor::RenderOverlay");

  if (!this->Context)
    {
    vtkErrorMacro(<< "vtkContextActor::Render - No painter set");
    return 0;
    }

  // This is the entry point for all 2D rendering.
  // First initialize the drawing device.
  this->Context->GetDevice()->Begin(viewport);

  int size[2];
  size[0] = viewport->GetSize()[0];
  size[1] = viewport->GetSize()[1];

  this->Scene->SetGeometry(&size[0]);

  this->Scene->Paint(this->Context);

  this->Context->GetDevice()->End();

  return 1;
}

//----------------------------------------------------------------------------
void vtkContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
    {
    this->Context->PrintSelf(os, indent.GetNextIndent());
    }
}
