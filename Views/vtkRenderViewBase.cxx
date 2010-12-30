/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderViewBase.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderViewBase.h"

#include "vtkCamera.h"
#include "vtkGenericRenderWindowInteractor.h"
#include "vtkInteractorObserver.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

vtkStandardNewMacro(vtkRenderViewBase);

vtkRenderViewBase::vtkRenderViewBase()
{
  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->RenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  this->RenderWindow->AddRenderer(this->Renderer);

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  this->SetInteractor(iren);
}

vtkRenderViewBase::~vtkRenderViewBase()
{
}

vtkRenderer* vtkRenderViewBase::GetRenderer()
{
  return this->Renderer;
}

void vtkRenderViewBase::SetRenderer(vtkRenderer* newren)
{
  vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  rens->InitTraversal(cookie);
  while(vtkRenderer *ren = rens->GetNextRenderer(cookie))
    {
    if (ren->GetLayer()<2)
      {
      ren->SetRenderWindow(NULL);
      this->RenderWindow->RemoveRenderer(ren);
      }
    }

  this->RenderWindow->AddRenderer(newren);
  this->Renderer = newren;
}

vtkRenderWindow* vtkRenderViewBase::GetRenderWindow()
{
  return this->RenderWindow;
}

void vtkRenderViewBase::SetRenderWindow(vtkRenderWindow* win)
{
  if (!win)
    {
    vtkErrorMacro(<< "SetRenderWindow called with a null window pointer."
                  << " That can't be right.");
    return;
    }

  // move renderers to new window
  vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  while(rens->GetNumberOfItems())
    {
    vtkRenderer* ren = rens->GetFirstRenderer();
    ren->SetRenderWindow(NULL);
    win->AddRenderer(ren);
    this->RenderWindow->RemoveRenderer(ren);
    }

  vtkSmartPointer<vtkInteractorObserver> style = this->GetInteractor()?
        this->GetInteractor()->GetInteractorStyle() : NULL;
  this->RenderWindow = win;
  if (this->GetInteractor())
    {
    this->GetInteractor()->SetInteractorStyle(style);
    }
  else if (style)
    {
    vtkGenericRenderWindowInteractor* iren =
        vtkGenericRenderWindowInteractor::New();
    win->SetInteractor(iren);
    iren->SetInteractorStyle(style);
    iren->Delete();
    }
}

vtkRenderWindowInteractor* vtkRenderViewBase::GetInteractor()
{
  return this->RenderWindow->GetInteractor();
}

void vtkRenderViewBase::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  if (interactor == this->GetInteractor())
    {
    return;
    }

  vtkSmartPointer<vtkInteractorObserver> style = this->GetInteractor() ?
        this->GetInteractor()->GetInteractorStyle() : NULL;
  this->RenderWindow->SetInteractor(interactor);

  if (this->GetInteractor())
    {
    this->GetInteractor()->SetInteractorStyle(style);
    }
  else if (style && this->RenderWindow)
    {
    vtkGenericRenderWindowInteractor* iren =
        vtkGenericRenderWindowInteractor::New();
    this->RenderWindow->SetInteractor(iren);
    iren->SetInteractorStyle(style);
    iren->Delete();
    }
}

void vtkRenderViewBase::Render()
{
  this->PrepareForRendering();
  this->RenderWindow->Render();
}

void vtkRenderViewBase::ResetCamera()
{
  this->PrepareForRendering();
  this->Renderer->ResetCamera();
}

void vtkRenderViewBase::ResetCameraClippingRange()
{
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
}

void vtkRenderViewBase::PrepareForRendering()
{
  this->Update();
}

void vtkRenderViewBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderWindow: ";
  if (this->RenderWindow)
    {
    os << "\n";
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
  os << indent << "Renderer: ";
  if (this->Renderer)
    {
    os << "\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}
