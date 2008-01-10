/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLDisplayListPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLDisplayListPainter.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkTimerLog.h"
#include "vtkWindow.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLDisplayListPainter);
vtkCxxRevisionMacro(vtkOpenGLDisplayListPainter, "1.5");
#endif
//-----------------------------------------------------------------------------
vtkOpenGLDisplayListPainter::vtkOpenGLDisplayListPainter()
{
  this->DisplayListId = 0;
}

//-----------------------------------------------------------------------------
vtkOpenGLDisplayListPainter::~vtkOpenGLDisplayListPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::ReleaseGraphicsResources(vtkWindow* win)
{
  if (win)
    {
    win->MakeCurrent();
    this->ReleaseList();
    }
  this->Superclass::ReleaseGraphicsResources(win);
  this->LastWindow = NULL;
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::ReleaseList()
{
  if (this->DisplayListId)
    {
    glDeleteLists(this->DisplayListId, 1);
    this->DisplayListId = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
  unsigned long typeflags)
{
  if (this->ImmediateModeRendering)
    {
    // don't use display lists at all.
    this->ReleaseGraphicsResources(renderer->GetRenderWindow());
    this->Superclass::RenderInternal(renderer, actor, typeflags);
    return;
    }

  this->TimeToDraw = 0.0;

  // if something has changed regenrate display lists.
  if (!this->DisplayListId || 
    this->GetMTime() > this->BuildTime ||
    this->GetInput()->GetMTime() > this->BuildTime ||
    actor->GetProperty()->GetMTime() > this->BuildTime ||
    this->Information->GetMTime() > this->BuildTime || 
    renderer->GetRenderWindow() != this->LastWindow.GetPointer())
    {
    this->ReleaseList();
    this->DisplayListId = glGenLists(1);
    glNewList(this->DisplayListId, GL_COMPILE);
    // generate the display list.
    this->Superclass::RenderInternal(renderer, actor, typeflags);
    glEndList();

    this->BuildTime.Modified();
    this->LastWindow = renderer->GetRenderWindow();
    }

  // Time the actual drawing.
  this->Timer->StartTimer();
  // render the display list.
  // if nothing has changed we use an old display list else
  // we use the newly generated list.
  glCallList(this->DisplayListId);
  // glFinish(); // To compute time correctly, we need to wait 
  // till OpenGL finishes.
  this->Timer->StopTimer();

  this->TimeToDraw += this->Timer->GetElapsedTime();
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
