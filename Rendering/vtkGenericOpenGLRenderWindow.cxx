/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkOpenGLRenderer.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkGenericOpenGLRenderWindow);

vtkGenericOpenGLRenderWindow::vtkGenericOpenGLRenderWindow()
{
}

vtkGenericOpenGLRenderWindow::~vtkGenericOpenGLRenderWindow()
{
  this->Finalize();

  vtkRenderer *ren;
  vtkCollectionSimpleIterator rit;
  this->Renderers->InitTraversal(rit);
  while ( (ren = this->Renderers->GetNextRenderer(rit)) )
    {
    ren->SetRenderWindow(NULL);
    }
}

void vtkGenericOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkGenericOpenGLRenderWindow::SetFrontBuffer(unsigned int b)
{
  this->FrontBuffer = b;
}

void vtkGenericOpenGLRenderWindow::SetFrontLeftBuffer(unsigned int b)
{
  this->FrontLeftBuffer = b;
}

void vtkGenericOpenGLRenderWindow::SetFrontRightBuffer(unsigned int b)
{
  this->FrontRightBuffer = b;
}

void vtkGenericOpenGLRenderWindow::SetBackBuffer(unsigned int b)
{
  this->BackBuffer = b;
}

void vtkGenericOpenGLRenderWindow::SetBackLeftBuffer(unsigned int b)
{
  this->BackLeftBuffer = b;
}

void vtkGenericOpenGLRenderWindow::SetBackRightBuffer(unsigned int b)
{
  this->BackRightBuffer = b;
}

void vtkGenericOpenGLRenderWindow::Finalize()
{
  // tell each of the renderers that this render window/graphics context
  // is being removed (the RendererCollection is removed by vtkRenderWindow's
  // destructor)
  vtkRenderer* ren;
  this->Renderers->InitTraversal();
  for ( ren = vtkOpenGLRenderer::SafeDownCast(this->Renderers->GetNextItemAsObject());
    ren != NULL;
    ren = vtkOpenGLRenderer::SafeDownCast(this->Renderers->GetNextItemAsObject())  )
    {
    ren->SetRenderWindow(NULL);
    ren->SetRenderWindow(this);
    }
}

void vtkGenericOpenGLRenderWindow::Frame()
{
  this->InvokeEvent(vtkCommand::WindowFrameEvent, NULL);
}

void vtkGenericOpenGLRenderWindow::MakeCurrent()
{
  this->InvokeEvent(vtkCommand::WindowMakeCurrentEvent, NULL);
}

bool vtkGenericOpenGLRenderWindow::IsCurrent()
{
  bool current = 0;
  this->InvokeEvent(vtkCommand::WindowIsCurrentEvent, &current);
  return current;
}

int vtkGenericOpenGLRenderWindow::SupportsOpenGL()
{
  int supports_ogl = 0;
  this->InvokeEvent(vtkCommand::WindowSupportsOpenGLEvent, &supports_ogl);
  return supports_ogl;
}

int vtkGenericOpenGLRenderWindow::IsDirect()
{
  int is_direct = 0;
  this->InvokeEvent(vtkCommand::WindowIsDirectEvent, &is_direct);
  return is_direct;
}

void vtkGenericOpenGLRenderWindow::PushState()
{
  glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

void vtkGenericOpenGLRenderWindow::PopState()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopClientAttrib();
  glPopAttrib();
}


void vtkGenericOpenGLRenderWindow::SetWindowId(void*)
{
}

void* vtkGenericOpenGLRenderWindow::GetGenericWindowId()
{
  return NULL;
}


void vtkGenericOpenGLRenderWindow::SetDisplayId(void*)
{
}

void vtkGenericOpenGLRenderWindow::SetParentId(void*)
{
}

void* vtkGenericOpenGLRenderWindow::GetGenericDisplayId()
{
  return NULL;
}

void* vtkGenericOpenGLRenderWindow::GetGenericParentId()
{
  return NULL;
}

void* vtkGenericOpenGLRenderWindow::GetGenericContext()
{
  return NULL;
}

void* vtkGenericOpenGLRenderWindow::GetGenericDrawable()
{
  return NULL;
}

void vtkGenericOpenGLRenderWindow::SetWindowInfo(char*)
{
}

void vtkGenericOpenGLRenderWindow::SetParentInfo(char*)
{
}

int* vtkGenericOpenGLRenderWindow::GetScreenSize()
{
  return NULL;
}

void vtkGenericOpenGLRenderWindow::Start()
{
}

void vtkGenericOpenGLRenderWindow::HideCursor()
{
}

void vtkGenericOpenGLRenderWindow::ShowCursor()
{
}

void vtkGenericOpenGLRenderWindow::SetFullScreen(int)
{
}

void vtkGenericOpenGLRenderWindow::WindowRemap()
{
}

int vtkGenericOpenGLRenderWindow::GetEventPending()
{
  return 0;
}

void vtkGenericOpenGLRenderWindow::SetNextWindowId(void*)
{
}

void vtkGenericOpenGLRenderWindow::SetNextWindowInfo(char*)
{
}

void vtkGenericOpenGLRenderWindow::CreateAWindow()
{
}

void vtkGenericOpenGLRenderWindow::DestroyWindow()
{
}
