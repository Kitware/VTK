/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCoincidentTopologyResolutionPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLCoincidentTopologyResolutionPainter.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMapper.h" // for VTK_RESOLVE_*
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLCoincidentTopologyResolutionPainter);
#endif
//-----------------------------------------------------------------------------
vtkOpenGLCoincidentTopologyResolutionPainter::
vtkOpenGLCoincidentTopologyResolutionPainter()
{
  
}

//-----------------------------------------------------------------------------
vtkOpenGLCoincidentTopologyResolutionPainter::
~vtkOpenGLCoincidentTopologyResolutionPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::RenderInternal(
   vtkRenderer *renderer,
   vtkActor *actor,
   unsigned long typeflags,
    bool forceCompileOnly)
{
  vtkProperty* prop = actor->GetProperty();
  bool draw_surface_with_edges =
    (prop->GetEdgeVisibility() && prop->GetRepresentation() == VTK_SURFACE);
  int rct = draw_surface_with_edges? VTK_RESOLVE_OFF: this->ResolveCoincidentTopology;
  switch (rct)
    {
  case VTK_RESOLVE_OFF:
    this->Superclass::RenderInternal(renderer, actor,
      typeflags, forceCompileOnly);
    break;

  case VTK_RESOLVE_POLYGON_OFFSET:
    this->RenderPolygonOffset(renderer, actor, typeflags, forceCompileOnly);
    break;

  case VTK_RESOLVE_SHIFT_ZBUFFER:
    this->RenderShiftZBuffer(renderer, actor, typeflags, forceCompileOnly);
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::RenderPolygonOffset(
                                                        vtkRenderer *renderer,
                                                        vtkActor *actor,
                                                        unsigned long typeflags,
                                                        bool forceCompileOnly)
{
#ifdef GL_VERSION_1_1
  if (this->OffsetFaces)
    {
    glEnable(GL_POLYGON_OFFSET_FILL);
    }
  else
    {
    glEnable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_POLYGON_OFFSET_POINT);
    }
  glPolygonOffset(this->PolygonOffsetFactor, this->PolygonOffsetUnits);
#endif      

  this->Superclass::RenderInternal(renderer, actor, typeflags,
                                   forceCompileOnly);

#ifdef GL_VERSION_1_1
  if (this->OffsetFaces)
    {
    glDisable(GL_POLYGON_OFFSET_FILL);
    }
  else
    {
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_POINT);
    }
#endif
}

//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::RenderShiftZBuffer(
                                                        vtkRenderer *renderer,
                                                        vtkActor *actor,
                                                        unsigned long typeflags,
                                                        bool forceCompileOnly)
{
  // Get the flags for each type of primitive.  Polygons can be drawn
  // as vertices or lines rather than filled, so check the property and
  // OpenGL flags to try to determine which one we are doing.
  unsigned long vertFlags = typeflags & vtkPainter::VERTS;
  unsigned long lineFlags = typeflags & vtkPainter::LINES;
  unsigned long polyFlags = 0;
  int actorRep = actor->GetProperty()->GetRepresentation();
  GLint oglPolyMode[2];
  glGetIntegerv(GL_POLYGON_MODE, oglPolyMode);
  if ((actorRep == VTK_POINTS) || (oglPolyMode[0] == GL_POINT))
    {
    vertFlags |= typeflags & (vtkPainter::POLYS | vtkPainter::STRIPS);
    }
  else if ((actorRep == VTK_WIREFRAME) || (oglPolyMode[0] == GL_LINE))
    {
    lineFlags |= typeflags & (vtkPainter::POLYS | vtkPainter::STRIPS);
    }
  else
    {
    polyFlags |= typeflags & (vtkPainter::POLYS | vtkPainter::STRIPS);
    }

  GLint stackDepth, maxStackDepth;
  glGetIntegerv(GL_PROJECTION_STACK_DEPTH, &stackDepth);
  glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH, &maxStackDepth);
  // We need to push the projection matrix on the stack.  Unfortunatly, the
  // projection matrix stack can be small, so we check to make sure that we
  // can do it.
  if (stackDepth < maxStackDepth)
    {
    double range[2];
    renderer->GetActiveCamera()->GetClippingRange(range);
    if (vertFlags)
      {
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glTranslated(0.0, 0.0, 2.0*this->ZShift*(range[1]-range[0]));
      this->Superclass::RenderInternal(renderer, actor, vertFlags,
                                       forceCompileOnly);
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      }
    if (lineFlags)
      {
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glTranslated(0.0, 0.0, this->ZShift*(range[1]-range[0]));
      this->Superclass::RenderInternal(renderer, actor, lineFlags,
                                       forceCompileOnly);
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      }      
    if (polyFlags)
      {
      this->Superclass::RenderInternal(renderer, actor, polyFlags,
                                       forceCompileOnly);
      }      
    }
  else
    {
    this->Superclass::RenderInternal(renderer, actor, typeflags,
                                     forceCompileOnly);
    }
}


//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
