/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLClipPlanesPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLClipPlanesPainter.h"

#include "vtkActor.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlaneCollection.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLClipPlanesPainter);
#endif
//-----------------------------------------------------------------------------
vtkOpenGLClipPlanesPainter::vtkOpenGLClipPlanesPainter()
{
}

//-----------------------------------------------------------------------------
vtkOpenGLClipPlanesPainter::~vtkOpenGLClipPlanesPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLClipPlanesPainter::RenderInternal(vtkRenderer *renderer, 
                                                vtkActor *actor,
                                                unsigned long typeflags,
                                                bool forceCompileOnly)
{
  vtkPlaneCollection *clipPlanes;
  vtkPlane *plane;
  int i, numClipPlanes;
  double planeEquation[4];

  clipPlanes = this->ClippingPlanes;

  if (clipPlanes == NULL)
    {
    numClipPlanes = 0;
    }
  else
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL has a limit of 6 clipping planes");
      numClipPlanes = 6;
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  if ( clipPlanes )
    {
    double *mat = *actor->GetMatrix()->Element;
    double origin[4], normal[3];

    for (i = 0; i < numClipPlanes; i++)
      {    
      plane = static_cast<vtkPlane *>(clipPlanes->GetItemAsObject(i));

      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      // Compute the plane equation
      double v1 = normal[0];
      double v2 = normal[1];
      double v3 = normal[2];
      double v4 = -(v1*origin[0] + v2*origin[1] + v3*origin[2]);

      // Transform normal from world to data coords
      planeEquation[0] = v1*mat[0] + v2*mat[4] + v3*mat[8]  + v4*mat[12];
      planeEquation[1] = v1*mat[1] + v2*mat[5] + v3*mat[9]  + v4*mat[13];
      planeEquation[2] = v1*mat[2] + v2*mat[6] + v3*mat[10] + v4*mat[14];
      planeEquation[3] = v1*mat[3] + v2*mat[7] + v3*mat[11] + v4*mat[15];

      glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0+i), planeEquation);
      }
    }

  this->Superclass::RenderInternal(renderer, actor, typeflags,forceCompileOnly);

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLClipPlanesPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
