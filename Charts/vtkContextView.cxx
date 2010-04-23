/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContextView.h"

#include "vtkContext2D.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkContextScene.h"

#include "vtkViewport.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#include "vtkContextActor.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkContextView);

vtkCxxSetObjectMacro(vtkContextView, Context, vtkContext2D);
vtkCxxSetObjectMacro(vtkContextView, Scene, vtkContextScene);

//----------------------------------------------------------------------------
vtkContextView::vtkContextView()
{
  this->Context = vtkContext2D::New();
  vtkOpenGLContextDevice2D *pd = vtkOpenGLContextDevice2D::New();
  this->Context->Begin(pd);
  pd->Delete();

  vtkContextActor *actor = vtkContextActor::New();
  this->Renderer->AddActor(actor);
  actor->Delete();
  this->Scene = actor->GetScene(); // We keep a pointer to this for convenience
  this->Scene->Register(this);
  // Should not need to do this...
  this->Scene->SetRenderer(this->Renderer);

  // Set up our view to render on move, 2D interaction style
  this->SetDisplayHoverText(false);
  this->RenderOnMouseMoveOn();
  this->SetInteractionModeTo2D();

  // Single color background
  this->Renderer->SetBackground(1.0, 1.0, 1.0);
  this->Renderer->SetBackground2(1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------
vtkContextView::~vtkContextView()
{
  if (this->Context)
    {
    this->Context->Delete();
    this->Context = NULL;
    }
  // The scene is owned by the context actor
  if (this->Scene)
    {
    this->Scene->Delete();
    this->Scene = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkContextView::SetInteractionMode(int mode)
{
  this->vtkRenderView::SetInteractionMode(mode);
  this->Scene->SetInteractorStyle(
      vtkInteractorStyle::SafeDownCast(this->RenderWindow->GetInteractor()->GetInteractorStyle()));
}

void vtkContextView::Render()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  this->RenderWindow->Render();

  // Render our scene
/*  this->Context->GetDevice()->Begin(this->Renderer);
  int size[2];
  size[0] = this->Renderer->GetSize()[0];
  size[1] = this->Renderer->GetSize()[1];
  this->Scene->SetGeometry(&size[0]);
  this->Scene->Paint(this->Context);
  this->Context->GetDevice()->End(); */
}

//----------------------------------------------------------------------------
void vtkContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
    {
    this->Context->PrintSelf(os, indent.GetNextIndent());
    }
}
