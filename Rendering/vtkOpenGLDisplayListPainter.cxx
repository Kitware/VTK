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

#include "vtkPolyData.h"
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
vtkCxxRevisionMacro(vtkOpenGLDisplayListPainter, "1.10");
#endif
//-----------------------------------------------------------------------------
vtkOpenGLDisplayListPainter::vtkOpenGLDisplayListPainter()
{
  this->DisplayListId = 0;
  this->LastUsedTypeFlags = 0;

  this->PDDisplayLists[0] = this->PDDisplayLists[1] =
    this->PDDisplayLists[2] = this->PDDisplayLists[3] = 0;
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
    this->ReleasePolyDataLists();
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
void vtkOpenGLDisplayListPainter::ReleasePolyDataLists()
{
  this->ReleaseList(0);
  this->ReleaseList(1);
  this->ReleaseList(2);
  this->ReleaseList(3);
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::ReleaseList(int idx)
{
  if (this->PDDisplayLists[idx])
    {
    glDeleteLists(this->PDDisplayLists[idx], 1);
    this->PDDisplayLists[idx] = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::RenderInternal(vtkRenderer *renderer,
                                                 vtkActor *actor, 
                                                 unsigned long typeflags,
                                                 bool forceCompileOnly)
{
  if (this->ImmediateModeRendering)
    {
    // don't use display lists at all.
    this->ReleaseGraphicsResources(renderer->GetRenderWindow());
    if (!forceCompileOnly)
      {
      this->Superclass::RenderInternal(renderer, actor, typeflags,
        forceCompileOnly);
      }
    return;
    }

  vtkPolyData* inputPD = vtkPolyData::SafeDownCast(this->GetInput());
  if (inputPD)
    {
    // release the non-polydata list
    this->ReleaseList();
    // render polydata
    this->RenderInternal(inputPD, renderer, actor, typeflags, forceCompileOnly);
    return;
    }

  // release the poly data lists
  this->ReleasePolyDataLists();

  // Now, render the non-polydata.
  
  this->TimeToDraw = 0.0;

  // if something has changed regenrate display lists.
  if (!this->DisplayListId || 
    this->GetMTime() > this->BuildTime ||
    this->GetInput()->GetMTime() > this->BuildTime ||
    actor->GetProperty()->GetMTime() > this->BuildTime ||
    renderer->GetRenderWindow() != this->LastWindow.GetPointer() ||
    this->Information->GetMTime() > this->BuildTime || 
    this->LastUsedTypeFlags != typeflags)
    {
    this->ReleaseList();
    this->DisplayListId = glGenLists(1);
    glNewList(this->DisplayListId, GL_COMPILE);
    // generate the display list.
    this->Superclass::RenderInternal(renderer, actor, typeflags,
                                     forceCompileOnly);
    glEndList();

    this->BuildTime.Modified();
    this->LastWindow = renderer->GetRenderWindow();
    this->LastUsedTypeFlags = typeflags;
    }

  if (!forceCompileOnly)
    {
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
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::RenderInternal(
  vtkPolyData* input,
  vtkRenderer *renderer,
  vtkActor *actor,
  unsigned long typeflags,
  bool forceCompileOnly)
{
  // Using display lists.
  unsigned long types[] = { vtkPainter::VERTS, vtkPainter::LINES,
    vtkPainter::POLYS, vtkPainter::STRIPS };
  int i;

  vtkIdType numCells[4];
  numCells[0] = input->GetNumberOfVerts();
  numCells[1] = input->GetNumberOfLines();
  numCells[2] = input->GetNumberOfPolys();
  numCells[3] = input->GetNumberOfStrips();

  vtkIdType total_cells = 0;
  for (i=0; i < 4; i++)
    {
    if (typeflags & types[i])
      {
      total_cells += numCells[i];
      }
    }
  if (total_cells == 0)
    {
    // nothing to render.
    return;
    }

  this->ProgressOffset = 0.0;

  this->TimeToDraw = 0.0;

  for (i = 0; i < 4; i++)
    {
    if (!(typeflags && types[i]))
      {
      // type not requested.
      continue;
      }
    this->ProgressScaleFactor = static_cast<double>(numCells[i])/total_cells;

    // if something has changed regenrate display lists.
    if (!this->PDDisplayLists[i] ||
      this->GetMTime() > this->PDBuildTimes[i] ||
      this->GetInput()->GetMTime() > this->PDBuildTimes[i] ||
      actor->GetProperty()->GetMTime() > this->PDBuildTimes[i]||
      renderer->GetRenderWindow() != this->LastWindow.GetPointer() ||
      this->Information->GetMTime() > this->PDBuildTimes[i])
      {
      this->ReleaseList(i);
      this->PDDisplayLists[i] = glGenLists(1);
      glNewList(this->PDDisplayLists[i], GL_COMPILE);
      // generate the display list.
      this->Superclass::RenderInternal(renderer, actor, typeflags,
        forceCompileOnly);
      glEndList();

      this->PDBuildTimes[i].Modified();
      this->LastWindow = renderer->GetRenderWindow();
      }

    if(!forceCompileOnly)
      {
      // Time the actual drawing.
      this->Timer->StartTimer();
      // render the display list.
      // if nothing has changed we use an old display list else
      // we use the newly generated list.
      glCallList(this->PDDisplayLists[i]);
      // glFinish(); // To compute time correctly, we need to wait
      // till OpenGL finishes.
      this->Timer->StopTimer();
      this->TimeToDraw += this->Timer->GetElapsedTime();
      }

    this->ProgressOffset += this->ProgressScaleFactor;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
