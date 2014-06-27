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
    XWindowAttributes attribs;
    // To avoid the expensive XGetWindowAttributes call,
    // compute window size at the start of a render and use
    // the ivar other times.
    XGetWindowAttributes(this->DisplayId,
                         this->WindowId, &attribs);
    this->SetSize(attribs.width, attribs.height);
    //std::cout << "Compute X window size " << attribs.width <<  "," << attribs.height << std::endl;
    }

  this->MakeCurrent();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Render()
{
//  GLint viewport[4];
//  // NOTE: We have to consider the size of the window as summation of
//  // viewport[2] + viewport[0] (and same for vertical dimension) because
//  // it is possible that this code gets called by external system twice.
//  // In the first case you may get (0, 0, 960, 1080) viewport dimensions and
//  // in the second case you will get (960, 0, 960, 1080). In the second case,
//  // the window diemsion needs to be 960+960 (initial pos + width) or else the
//  // rendering on the second viewport won't show anything.
//  glGetIntegerv(GL_VIEWPORT, viewport);
//  this->SetSize(viewport[2] + viewport[0], viewport[3] + viewport[1]);
//std::cout << "Renwin Render size " << this->Size[0] << "," << this->Size[1] << std::endl;

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
      renderer->GetActiveCamera()->SetLeftEye(0);
      }
    else
      {
      renderer->GetActiveCamera()->SetLeftEye(1);
      }
    }
  this->Superclass::Render();
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
