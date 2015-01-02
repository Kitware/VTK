/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExternalVTKWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "ExternalVTKWidget.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(ExternalVTKWidget);

//----------------------------------------------------------------------------
ExternalVTKWidget::ExternalVTKWidget()
{
  this->RenderWindow = 0;
//  this->RenderWindow = vtkExternalOpenGLRenderWindow::New();
  this->Renderer = vtkExternalOpenGLRenderer::New();
//  this->RenderWindow->AddRenderer(this->Renderer);
//  this->Interactor = 0;
//  this->Interactor = vtkGenericRenderWindowInteractor::New();
//  this->Interactor->SetRenderWindow(this->RenderWindow);
}

//----------------------------------------------------------------------------
ExternalVTKWidget::~ExternalVTKWidget()
{
  this->SetRenderWindow(NULL);
  this->Renderer->Delete();
//  this->RenderWindow->Delete();
//  this->Interactor->Delete();
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow* ExternalVTKWidget::GetRenderWindow()
{
  if (!this->RenderWindow)
    {
    vtkExternalOpenGLRenderWindow * win = vtkExternalOpenGLRenderWindow::New();
    this->SetRenderWindow(win);
    win->Delete();
    }
  return this->RenderWindow;
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderer* ExternalVTKWidget::GetRenderer()
{
  return this->Renderer;
}

////----------------------------------------------------------------------------
//vtkGenericRenderWindowInteractor* ExternalVTKWidget::GetInteractor()
//{
//  return this->Interactor;
//}

//----------------------------------------------------------------------------
void ExternalVTKWidget::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void ExternalVTKWidget::SetRenderWindow(vtkExternalOpenGLRenderWindow * w)
{
  // Do nothing if we don't have to
  if (this->RenderWindow == w)
    {
    return;
    }

  // Unregister the previous window
  if (this->RenderWindow)
    {
    this->RenderWindow->Finalize();
    this->RenderWindow->SetMapped(0);
    this->RenderWindow->UnRegister(this);
    }

  // Now, set the new window
  this->RenderWindow = w;

  if (this->RenderWindow)
    {
    // If it is mapped somewhere else, unmap it.
    this->RenderWindow->Finalize();
    this->RenderWindow->SetMapped(1);
    this->RenderWindow->Register(this);

    // Add the renderer
    this->RenderWindow->AddRenderer(this->Renderer);
    }
}
