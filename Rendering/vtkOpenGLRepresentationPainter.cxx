/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRepresentationPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRepresentationPainter.h"

#include "vtkActor.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLRepresentationPainter);
vtkCxxRevisionMacro(vtkOpenGLRepresentationPainter, "1.1.2.2");
#endif

//-----------------------------------------------------------------------------
vtkOpenGLRepresentationPainter::vtkOpenGLRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
vtkOpenGLRepresentationPainter::~vtkOpenGLRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLRepresentationPainter::RenderInternal(vtkRenderer* renderer, 
  vtkActor* actor, unsigned long typeflags)
{
  vtkProperty* prop = actor->GetProperty();
  int rep = prop->GetRepresentation();
  int reset_needed = 0;
  if (!prop->GetBackfaceCulling() && !prop->GetFrontfaceCulling())
    {
    switch (rep)
      {
    case VTK_POINTS:
      glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
      reset_needed = 1;
      break;
    case VTK_WIREFRAME:
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      reset_needed = 1;
      break;
      }
    }
  this->Superclass::RenderInternal(renderer, actor, typeflags);
  if (reset_needed)
    {
    // reset the default.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}
//-----------------------------------------------------------------------------
void vtkOpenGLRepresentationPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
