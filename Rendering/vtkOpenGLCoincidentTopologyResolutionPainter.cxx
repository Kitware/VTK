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

#include "vtkMapper.h" // for VTK_RESOLVE_*
#include "vtkObjectFactory.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#  if defined(__APPLE__) && (defined(VTK_USE_CARBON) || defined(VTK_USE_COCOA))
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

vtkStandardNewMacro(vtkOpenGLCoincidentTopologyResolutionPainter);
vtkCxxRevisionMacro(vtkOpenGLCoincidentTopologyResolutionPainter, 
  "1.1.2.1");
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
  vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags)
{
  int resolve=0, zResolve=0;
  double zRes = 0.0;
  if ( this->ResolveCoincidentTopology )
    {
    resolve = 1;
    if ( this->ResolveCoincidentTopology == VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      zResolve = 1;
      zRes = this->ZShift;
      }
    else
      {
#ifdef GL_VERSION_1_1
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(this->PolygonOffsetFactor, this->PolygonOffsetUnits);
#endif      
      }
    }
  
  this->Superclass::RenderInternal(renderer, actor, typeflags);

  if (resolve)
    {
    if ( zResolve )
      {
      glDepthRange(0., 1.);
      }
    else
      {
#ifdef GL_VERSION_1_1
      glDisable(GL_POLYGON_OFFSET_FILL);
#endif
      }
    }
}


//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
