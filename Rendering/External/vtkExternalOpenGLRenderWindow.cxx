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

#include "vtkCamera.h"
#include "vtkExternalOpenGLRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkExternalOpenGLRenderWindow);

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::vtkExternalOpenGLRenderWindow()
{
  this->Initialized = false;
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::~vtkExternalOpenGLRenderWindow()
{
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Start(void)
{
  if (!this->Initialized)
    {
    this->InitializeFromCurrentContext();
    this->Initialized = true;
    }
//  if (!this->DisplayId || !this->WindowId)
//    {
//    this->InitializeFromCurrentContext();
//    XWindowAttributes attribs;
//    // To avoid the expensive XGetWindowAttributes call,
//    // compute window size at the start of a render and use
//    // the ivar other times.
//    XGetWindowAttributes(this->DisplayId,
//                         this->WindowId, &attribs);
//    this->SetSize(attribs.width, attribs.height);
//    }
//
  this->MakeCurrent();

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
