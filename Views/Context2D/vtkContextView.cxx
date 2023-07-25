// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContextView.h"

#include "vtkContext2D.h"
#include "vtkContextDevice2D.h"
#include "vtkContextScene.h"

#include "vtkContextActor.h"
#include "vtkContextInteractorStyle.h"
#include "vtkInteractorStyle.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkViewport.h"

#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkContextView);

vtkCxxSetObjectMacro(vtkContextView, Context, vtkContext2D);
vtkCxxSetObjectMacro(vtkContextView, Scene, vtkContextScene);

//------------------------------------------------------------------------------
vtkContextView::vtkContextView()
{
  this->Context = vtkSmartPointer<vtkContext2D>::New();
  vtkNew<vtkContextDevice2D> pd;
  this->Context->Begin(pd);

  vtkContextActor* actor = vtkContextActor::New();
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

//------------------------------------------------------------------------------
vtkContextView::~vtkContextView() = default;

//------------------------------------------------------------------------------
vtkContext2D* vtkContextView::GetContext()
{
  return this->Context;
}

//------------------------------------------------------------------------------
vtkContextScene* vtkContextView::GetScene()
{
  return this->Scene;
}

//------------------------------------------------------------------------------
void vtkContextView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Context: " << this->Context << "\n";
  if (this->Context)
  {
    this->Context->PrintSelf(os, indent.GetNextIndent());
  }
}
VTK_ABI_NAMESPACE_END
