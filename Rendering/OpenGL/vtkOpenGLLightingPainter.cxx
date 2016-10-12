/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLightingPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLLightingPainter.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"

#include "vtkOpenGL.h"

vtkStandardNewMacro(vtkOpenGLLightingPainter);

//-----------------------------------------------------------------------------
static inline int vtkOpenGLLightingPainterGetTotalCells(vtkPolyData* pd,
  unsigned long typeflags)
{
  int total_cells = 0;
  total_cells += (typeflags & vtkPainter::VERTS)?
    pd->GetNumberOfVerts() : 0;
  total_cells += (typeflags & vtkPainter::LINES)?
    pd->GetNumberOfLines() : 0;
  total_cells += (typeflags & vtkPainter::POLYS)?
    pd->GetNumberOfPolys() : 0;
  total_cells += (typeflags & vtkPainter::STRIPS)?
    pd->GetNumberOfStrips() : 0;
  return total_cells;
}

//-----------------------------------------------------------------------------
vtkOpenGLLightingPainter::vtkOpenGLLightingPainter()
{
}

//-----------------------------------------------------------------------------
vtkOpenGLLightingPainter::~vtkOpenGLLightingPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLLightingPainter::RenderInternal(vtkRenderer *renderer,
                                              vtkActor *actor,
                                              unsigned long typeflags,
                                              bool forceCompileOnly)
{
  vtkPolyData* input = this->GetInputAsPolyData();
  vtkProperty* prop = actor->GetProperty();
  vtkDataArray* n = input->GetPointData()->GetNormals();

  // get the representation (e.g., surface / wireframe / points)
  int rep = prop->GetRepresentation();

  // get the shading interpolation
  int interpolation = prop->GetInterpolation();

  if (interpolation == VTK_FLAT)
  {
    n = 0;
  }

  if (n == 0)
  {
    n = input->GetCellData()->GetNormals();
  }

  unsigned long enable_flags = typeflags;
  unsigned long disable_flags = 0x0;

  if (rep == VTK_POINTS && !n)
  {
    disable_flags = typeflags;
    enable_flags = 0;
  }
  else if (!n &&
    ((typeflags & vtkPainter::VERTS) || (typeflags & vtkPainter::LINES)))
  {
    disable_flags = typeflags & (vtkPainter::VERTS | vtkPainter::LINES);
    enable_flags = typeflags & (~disable_flags);
  }

  int total_cells =
    vtkOpenGLLightingPainterGetTotalCells(input, typeflags);

  if (total_cells == 0)
  {
    // nothing to render.
    return;
  }

  this->ProgressOffset = 0.0;
  double time_to_draw = 0.0;
  if (disable_flags)
  {
    int disabled_cells = vtkOpenGLLightingPainterGetTotalCells(input,
      disable_flags);
    this->ProgressScaleFactor =
      static_cast<double>(disabled_cells) / total_cells;

    glDisable(GL_LIGHTING);
    this->Superclass::RenderInternal(renderer, actor, disable_flags,
                                     forceCompileOnly);
    time_to_draw += this->DelegatePainter?
      this->DelegatePainter->GetTimeToDraw() : 0;

    glEnable(GL_LIGHTING);

    this->ProgressOffset += this->ProgressScaleFactor;
  }

  if (enable_flags)
  {
    int enabled_cells = vtkOpenGLLightingPainterGetTotalCells(input,
      enable_flags);
    this->ProgressScaleFactor =
      static_cast<double>(enabled_cells) / total_cells;

    if(actor->GetProperty()->GetLighting()) // fixed-pipeline
    {
      glEnable(GL_LIGHTING);
    }
    else
    {
      glDisable(GL_LIGHTING);
    }
    this->Superclass::RenderInternal(renderer, actor, enable_flags,
                                      forceCompileOnly);

    time_to_draw += this->DelegatePainter?
      this->DelegatePainter->GetTimeToDraw() : 0;
  }

  this->TimeToDraw = time_to_draw;
}

//-----------------------------------------------------------------------------
void vtkOpenGLLightingPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
