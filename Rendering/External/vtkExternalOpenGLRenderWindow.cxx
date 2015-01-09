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
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"

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
  // For stereo, render the correct eye based on the OpenGL buffer mode
  GLint bufferType;
  glGetIntegerv(GL_DRAW_BUFFER, &bufferType);
  vtkCollectionSimpleIterator sit;
  vtkRenderer* renderer;
  for (this->GetRenderers()->InitTraversal(sit);
    (renderer = this->GetRenderers()->GetNextRenderer(sit)); )
    {
    if (bufferType == GL_BACK_RIGHT || bufferType == GL_RIGHT
      || bufferType == GL_FRONT_RIGHT)
      {
      this->StereoRenderOn();
      this->SetStereoTypeToRight();
      }
    else
      {
      this->SetStereoTypeToLeft();
      }
    }
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Render()
{
  this->Superclass::Render();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
