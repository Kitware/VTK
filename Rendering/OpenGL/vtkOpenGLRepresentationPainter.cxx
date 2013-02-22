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

#include "vtkOpenGL.h"

vtkStandardNewMacro(vtkOpenGLRepresentationPainter);

//-----------------------------------------------------------------------------
vtkOpenGLRepresentationPainter::vtkOpenGLRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
vtkOpenGLRepresentationPainter::~vtkOpenGLRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLRepresentationPainter::RenderInternal(vtkRenderer *renderer,
                                                    vtkActor *actor,
                                                    unsigned long typeflags,
                                                    bool forceCompileOnly)
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

  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
  if (draw_surface_with_edges)
    {
    glPolygonOffset(0.7, 1.0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    }

  this->Superclass::RenderInternal(renderer, actor, typeflags,
                                   forceCompileOnly);
  if (draw_surface_with_edges)
    {
    glDisable(GL_POLYGON_OFFSET_FILL);
    }
  this->TimeToDraw += this->DelegatePainter?
    this->DelegatePainter->GetTimeToDraw() : 0;
  if (reset_needed)
    {
    // reset the default.
    glPolygonMode(face, GL_FILL);
    }

  if (draw_surface_with_edges)
    {
    glPushAttrib(GL_CURRENT_BIT|GL_LIGHTING_BIT|GL_ENABLE_BIT);
    double color[4];
    prop->GetEdgeColor(color);
    color[0] *= prop->GetOpacity();
    color[1] *= prop->GetOpacity();
    color[2] *= prop->GetOpacity();
    color[3] = prop->GetOpacity();

    glDisable(GL_LIGHTING);
    glColor4dv(color);
    glPolygonMode(face, GL_LINE);

    // Disable textures when rendering the surface edges.
    // This ensures that edges are always drawn solid.
    glDisable(GL_TEXTURE_2D);

    this->Information->Set(vtkPolyDataPainter::DISABLE_SCALAR_COLOR(), 1);
    this->Superclass::RenderInternal(renderer, actor, typeflags,
                                     forceCompileOnly);
    this->TimeToDraw += this->DelegatePainter?
      this->DelegatePainter->GetTimeToDraw() : 0;
    this->Information->Remove(vtkPolyDataPainter::DISABLE_SCALAR_COLOR());

    // reset the default.
    glPolygonMode(face, GL_FILL);

    glPopAttrib(); //(GL_CURRENT_BIT|GL_LIGHTING|GL_ENABLE_BIT)
    }
}
//-----------------------------------------------------------------------------
void vtkOpenGLRepresentationPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
