/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExternalOpenGLRenderWindow.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkExternalOpenGLRenderWindow);

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::vtkExternalOpenGLRenderWindow()
{
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::~vtkExternalOpenGLRenderWindow()
{
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Start(void)
{
  if (!this->DisplayId || !this->WindowId)
    {
    this->InitializeFromCurrentContext();
    }

  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Render()
{
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  this->SetSize(viewport[2], viewport[3]);
  this->Superclass::Render();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "DrawWindow: " << this->DrawWindow << "\n";
  this->Superclass::PrintSelf(os, indent);
}
