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

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
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
vtkCxxRevisionMacro(vtkOpenGLDisplayListPainter, "1.2");
#endif
//-----------------------------------------------------------------------------
vtkOpenGLDisplayListPainter::vtkOpenGLDisplayListPainter()
{
  this->ListIds[0] = this->ListIds[1] = this->ListIds[2] = this->ListIds[3] = 0;
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
    for (int i=0; i < 4 ; i++)
      {
      this->ReleaseList(i);
      }
    }
  this->LastWindow = NULL;

  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::ReleaseList(int index)
{
  if (this->ListIds[index])
    {
    glDeleteLists(this->ListIds[index], 1);
    this->ListIds[index] = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
  unsigned long typeflags)
{
  int i;
  if (this->ImmediateModeRendering)
    {
    // don't use display lists at all.
    this->ReleaseGraphicsResources(renderer->GetRenderWindow());
    this->Superclass::RenderInternal(renderer, actor, typeflags);
    return;
    }

  // Using display lists.
  vtkPolyData* input = this->GetPolyData();
  unsigned long types[] = { vtkPainter::VERTS, vtkPainter::LINES, 
    vtkPainter::POLYS, vtkPainter::STRIPS };
 
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
  for (i=0; i < 4; i++)
    {
    if ( !(typeflags & types[i]) )
      {
      // type not requested.
      continue;
      }
    
    this->ProgressScaleFactor = static_cast<double>(numCells[i]) / total_cells;
    
    // if something has changed regenrate display lists.
    if (!this->ListIds[i] || 
      this->GetMTime() > this->BuildTimes[i] ||
      input->GetMTime() > this->BuildTimes[i] ||
      actor->GetProperty()->GetMTime() > this->BuildTimes[i] ||
      renderer->GetRenderWindow() != this->LastWindow)
      {
      this->ReleaseList(i);
      this->ListIds[i] = glGenLists(1);
      glNewList(this->ListIds[i], GL_COMPILE);
      // generate the display list.
      this->Superclass::RenderInternal(renderer, actor, types[i]);
      glEndList();

      this->BuildTimes[i].Modified();
      this->LastWindow = renderer->GetRenderWindow();
      }

    // Time the actual drawing.
    this->Timer->StartTimer();
    // render the display list.
    // if nothing has changed we use an old display list else
    // we use the newly generated list.
    glCallList(this->ListIds[i]);
    
    this->Timer->StopTimer();
    this->TimeToDraw += this->Timer->GetElapsedTime();
    
    this->ProgressOffset += this->ProgressScaleFactor;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLDisplayListPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
