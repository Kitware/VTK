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
#include "vtkContextInteractorStyle.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkContextView);

vtkCxxSetObjectMacro(vtkContextView, Context, vtkContext2D);
vtkCxxSetObjectMacro(vtkContextView, Scene, vtkContextScene);

//----------------------------------------------------------------------------
vtkContextView::vtkContextView()
{
  this->Context = vtkSmartPointer<vtkContext2D>::New();
  vtkOpenGLContextDevice2D *pd = vtkOpenGLContextDevice2D::New();
  this->Context->Begin(pd);
  pd->Delete();

  vtkContextActor *actor = vtkContextActor::New();
  this->Renderer->AddActor(actor);
  actor->Delete();
  this->Scene = actor->GetScene(); // We keep a pointer to this for convenience
  // Should not need to do this...
  this->Scene->SetRenderer(this->Renderer);

  vtkContextInteractorStyle* style = vtkContextInteractorStyle::New();
  style->SetScene(this->Scene);
  this->GetInteractor()->SetInteractorStyle(style);
  style->Delete();

  // Single color background by default.
  this->Renderer->SetBackground(1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------
vtkContextView::~vtkContextView()
{
}

//----------------------------------------------------------------------------
vtkContext2D* vtkContextView::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
vtkContextScene* vtkContextView::GetScene()
{
  return this->Scene;
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
