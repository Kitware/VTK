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
  this->RenderWindow = vtkExternalOpenGLRenderWindow::New();
  this->Renderer = vtkExternalOpenGLRenderer::New();
  this->RenderWindow->AddRenderer(this->Renderer);
  this->Interactor = vtkGenericRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this->RenderWindow);
}

//----------------------------------------------------------------------------
ExternalVTKWidget::~ExternalVTKWidget()
{
  this->Renderer->Delete();
  this->RenderWindow->Delete();
  this->Interactor->Delete();
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow* ExternalVTKWidget::GetRenderWindow()
{
  return this->RenderWindow;
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderer* ExternalVTKWidget::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkGenericRenderWindowInteractor* ExternalVTKWidget::GetInteractor()
{
  return this->Interactor;
}

//----------------------------------------------------------------------------
void ExternalVTKWidget::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
