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
#include "vtkCommand.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkRenderViewBase);

vtkRenderViewBase::vtkRenderViewBase()
{
  this->Renderer = vtkRenderer::New();
  this->RenderWindow = vtkRenderWindow::New();
  this->RenderWindow->AddRenderer(this->Renderer);

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->EnableRenderOff();
  iren->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
  iren->AddObserver(vtkCommand::StartInteractionEvent, this->GetObserver());
  iren->AddObserver(vtkCommand::EndInteractionEvent, this->GetObserver());
  this->RenderWindow->SetInteractor(iren);
}

vtkRenderViewBase::~vtkRenderViewBase()
{
  if (this->Renderer)
    {
    this->Renderer->Delete();
    }
  if (this->RenderWindow)
    {
    this->RenderWindow->Delete();
    }
}

void vtkRenderViewBase::SetRenderWindow(vtkRenderWindow* win)
{
  if (!win)
    {
    vtkErrorMacro(<< "SetRenderWindow called with a null window pointer."
                  << " That can't be right.");
    return;
    }

  // get rid of the render observer on the current window
  if (this->RenderWindow)
    {
    this->RenderWindow->RemoveObserver(this->GetObserver());
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

  // move interactor to new window
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
      this->RenderWindow->GetInteractor();
  this->RenderWindow->SetInteractor(NULL);
  iren->SetRenderWindow(NULL);
  win->SetInteractor(iren);
  iren->SetRenderWindow(win);

  this->RenderWindow->UnRegister(this);
  this->RenderWindow = win;
  this->RenderWindow->Register(this);
  this->RenderWindow->AddObserver(vtkCommand::EndEvent, this->GetObserver());
}

vtkRenderWindowInteractor* vtkRenderViewBase::GetInteractor()
{
  return this->RenderWindow->GetInteractor();
}

void vtkRenderViewBase::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  if (!interactor)
    {
    vtkErrorMacro(<< "SetInteractor called with a null interactor pointer."
                  << " That can't be right.");
    return;
    }

  // get rid of the render observer on any current interactor
  if (this->RenderWindow->GetInteractor())
    {
    this->RenderWindow->GetInteractor()->RemoveObserver(this->GetObserver());
    }

  // We will handle all interactor renders by turning off rendering
  // in the interactor and listening to the interactor's render event.
  interactor->EnableRenderOff();
  interactor->AddObserver(vtkCommand::RenderEvent, this->GetObserver());
  interactor->AddObserver(vtkCommand::StartInteractionEvent, this->GetObserver());
  interactor->AddObserver(vtkCommand::EndInteractionEvent, this->GetObserver());
  this->RenderWindow->SetInteractor(interactor);
}

void vtkRenderViewBase::Render()
{
  // Indirectly call this->RenderWindow->Start() without crashing.
  // to create context if it is not yet created and to make it current
  // this is required for HoverWidget to be active after the first
  // render.
  this->RenderWindow->GetInteractor()->Initialize();

  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCameraClippingRange();
  this->RenderWindow->Render();
}

void vtkRenderViewBase::ResetCamera()
{
  this->Update();
  this->PrepareForRendering();
  this->Renderer->ResetCamera();
}

void vtkRenderViewBase::ResetCameraClippingRange()
{
  this->Update();
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
