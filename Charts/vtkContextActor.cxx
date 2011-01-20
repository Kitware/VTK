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
#include "vtkOpenGL2ContextDevice2D.h"
#include "vtkContextScene.h"
#include "vtkTransform2D.h"

#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkContextActor);

vtkCxxSetObjectMacro(vtkContextActor, Context, vtkContext2D);
vtkCxxSetObjectMacro(vtkContextActor, Scene, vtkContextScene);

//----------------------------------------------------------------------------
vtkContextActor::vtkContextActor()
{
  this->Context = vtkContext2D::New();
  this->Scene = vtkContextScene::New();
  this->Initialized = false;
}

//----------------------------------------------------------------------------
// Destroy an actor2D.
vtkContextActor::~vtkContextActor()
{
  if (this->Context)
    {
    this->Context->End();
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
void vtkContextActor::ReleaseGraphicsResources(vtkWindow *window)
{
  vtkOpenGLContextDevice2D *device =
      vtkOpenGLContextDevice2D::SafeDownCast(this->Context->GetDevice());
  if (device)
    {
    device->ReleaseGraphicsResources(window);
    }

  if(this->Scene)
    {
    this->Scene->ReleaseGraphicsResources();
    }
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

  // Need to figure out how big the window is, taking into account tiling...
  vtkWindow *window = viewport->GetVTKWindow();
  int scale[2];
  window->GetTileScale(scale);
  int size[2];
  size[0] = window->GetSize()[0];
  size[1] = window->GetSize()[1];

  int viewportInfo[4];
  viewport->GetTiledSizeAndOrigin( &viewportInfo[0], &viewportInfo[1],
    &viewportInfo[2], &viewportInfo[3] );

  // The viewport is in normalized coordinates, and is the visible section of
  // the scene.
  vtkTransform2D* transform = this->Scene->GetTransform();
  transform->Identity();
  if (scale[0] > 1 || scale[1] > 1)
    {
    // Tiled display - work out the transform required
    double *b = window->GetTileViewport();
    int box[] = { vtkContext2D::FloatToInt(b[0] * size[0]),
                  vtkContext2D::FloatToInt(b[1] * size[1]),
                  vtkContext2D::FloatToInt(b[2] * size[0]),
                  vtkContext2D::FloatToInt(b[3] * size[1]) };
    transform->Translate(-box[0], -box[1]);
    if (this->Scene->GetScaleTiles())
      {
      transform->Scale(scale[0], scale[1]);
      }
    }
  else if (viewportInfo[0] != size[0] || viewportInfo[1] != size[1] )
    {
    size[0]=viewportInfo[0];
    size[1]=viewportInfo[1];
    }

  if (!this->Initialized)
    {
    this->Initialize(viewport);
    }

  // This is the entry point for all 2D rendering.
  // First initialize the drawing device.
  this->Context->GetDevice()->Begin(viewport);
  this->Scene->SetGeometry(size);
  this->Scene->Paint(this->Context);
  this->Context->GetDevice()->End();

  return 1;
}

//----------------------------------------------------------------------------
void vtkContextActor::Initialize(vtkViewport* viewport)
{
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
void vtkContextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
    {
    this->Context->PrintSelf(os, indent.GetNextIndent());
    }
}
