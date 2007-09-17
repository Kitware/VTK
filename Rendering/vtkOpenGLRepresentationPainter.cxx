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
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkPrimitivePainter.h"
#include "vtkProperty.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLRepresentationPainter);
vtkCxxRevisionMacro(vtkOpenGLRepresentationPainter, "1.4");
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


  GLenum face = GL_FRONT_AND_BACK;

  // If both front & back culling is on, will fall into backface culling.
  if (prop->GetBackfaceCulling())
    {
    face = GL_FRONT;
    }
  else if (prop->GetFrontfaceCulling())
    {
    face = GL_BACK;
    }

  switch (rep)
    {
  case VTK_POINTS:
    glPolygonMode(face, GL_POINT);
    reset_needed = 1;
    break;
  case VTK_WIREFRAME:
    glPolygonMode(face, GL_LINE);
    reset_needed = 1;
    break;
    }

  this->Superclass::RenderInternal(renderer, actor, typeflags);
  if (reset_needed)
    {
    // reset the default.
    glPolygonMode(face, GL_FILL);
    }

  if (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE)
    {
    glPushAttrib(GL_CURRENT_BIT);
    glPushAttrib(GL_LIGHTING_BIT);
    double color[4];
    prop->GetEdgeColor(color);
    color[0] *= prop->GetOpacity();
    color[1] *= prop->GetOpacity();
    color[2] *= prop->GetOpacity();
    color[3] = prop->GetOpacity();

    glDisable(GL_LIGHTING);
    glColor4dv(color);
    glPolygonMode(face, GL_LINE);

    this->Information->Set(vtkPrimitivePainter::DISABLE_SCALAR_COLOR(), 1);
    this->Superclass::RenderInternal(renderer, actor, typeflags);
    this->Information->Remove(vtkPrimitivePainter::DISABLE_SCALAR_COLOR());

    // reset the default.
    glPolygonMode(face, GL_FILL);

    glPopAttrib(); //GL_LIGHTING
    glPopAttrib(); //GL_CURRENT_BIT
    }
}
//-----------------------------------------------------------------------------
void vtkOpenGLRepresentationPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
