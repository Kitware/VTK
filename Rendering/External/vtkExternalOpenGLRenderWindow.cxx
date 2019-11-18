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

#include "vtk_glew.h"

#include "vtkExternalOpenGLRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLState.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

vtkStandardNewMacro(vtkExternalOpenGLRenderWindow);

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::vtkExternalOpenGLRenderWindow()
{
  this->AutomaticWindowPositionAndResize = 1;
  this->UseExternalContent = true;
}

//----------------------------------------------------------------------------
vtkExternalOpenGLRenderWindow::~vtkExternalOpenGLRenderWindow() {}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::Start(void)
{
  // Make sure all important OpenGL options are set for VTK
  this->OpenGLInit();

  // Use hardware acceleration
  this->SetIsDirect(1);

  auto ostate = this->GetState();

  if (this->AutomaticWindowPositionAndResize)
  {
    int info[4];
    ostate->vtkglGetIntegerv(GL_VIEWPORT, info);
    this->SetPosition(info[0], info[1]);
    this->SetSize(info[2], info[3]);
  }

  // creates or resizes the framebuffer
  this->Size[0] = (this->Size[0] > 0 ? this->Size[0] : 300);
  this->Size[1] = (this->Size[1] > 0 ? this->Size[1] : 300);
  this->CreateOffScreenFramebuffer(this->Size[0], this->Size[1]);

  // For stereo, render the correct eye based on the OpenGL buffer mode
  GLint bufferType;
  ostate->vtkglGetIntegerv(GL_DRAW_BUFFER, &bufferType);
  vtkCollectionSimpleIterator sit;
  vtkRenderer* renderer;
  for (this->GetRenderers()->InitTraversal(sit);
       (renderer = this->GetRenderers()->GetNextRenderer(sit));)
  {
    if (bufferType == GL_BACK_RIGHT || bufferType == GL_RIGHT || bufferType == GL_FRONT_RIGHT)
    {
      this->StereoRenderOn();
      this->SetStereoTypeToRight();
    }
    else
    {
      this->SetStereoTypeToLeft();
    }
  }

  ostate->PushFramebufferBindings();

  if (this->UseExternalContent)
  {
    const int destExtents[4] = { 0, this->Size[0], 0, this->Size[1] };
    this->OffScreenFramebuffer->Bind(GL_DRAW_FRAMEBUFFER);
    this->GetState()->vtkglViewport(0, 0, this->Size[0], this->Size[1]);
    this->GetState()->vtkglScissor(0, 0, this->Size[0], this->Size[1]);
    vtkOpenGLFramebufferObject::Blit(
      destExtents, destExtents, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  }

  this->OffScreenFramebuffer->Bind();
}

//----------------------------------------------------------------------------
bool vtkExternalOpenGLRenderWindow::IsCurrent(void)
{
  return true;
}

//----------------------------------------------------------------------------
void vtkExternalOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "UseExternalContent: " << this->UseExternalContent << endl;
  this->Superclass::PrintSelf(os, indent);
}
