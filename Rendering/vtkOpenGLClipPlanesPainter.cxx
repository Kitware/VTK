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
vtkCxxRevisionMacro(vtkOpenGLClipPlanesPainter, "1.1.2.2");
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
void vtkOpenGLClipPlanesPainter::RenderInternal(vtkRenderer* renderer, 
  vtkActor* actor, unsigned long typeflags)
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
      vtkErrorMacro(<< "OpenGL guarantees at most 6 additional clipping planes");
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glEnable((GLenum)(GL_CLIP_PLANE0+i));
    }

  if ( clipPlanes )
    {
    vtkMatrix4x4 *actorMatrix = vtkMatrix4x4::New();
    actor->GetMatrix( actorMatrix );
    actorMatrix->Invert();

    double origin[4], normal[3], point[4];

    for (i = 0; i < numClipPlanes; i++)
      {    
      plane = (vtkPlane *)clipPlanes->GetItemAsObject(i);

      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      point[0] = origin[0] + normal[0];
      point[1] = origin[1] + normal[1];
      point[2] = origin[2] + normal[2];

      origin[3] = point[3] = 1.0;

      actorMatrix->MultiplyPoint( origin, origin );
      actorMatrix->MultiplyPoint( point, point );

      if ( origin[3] != 1.0 )
        {
        origin[0] /= origin[3];
        origin[1] /= origin[3];
        origin[2] /= origin[3];
        }

      if ( point[3] != 1.0 )
        {
        point[0] /= point[3];
        point[1] /= point[3];
        point[2] /= point[3];
        }

      normal[0] = point[0] - origin[0];
      normal[1] = point[1] - origin[1];
      normal[2] = point[2] - origin[2];

      planeEquation[0] = normal[0];
      planeEquation[1] = normal[1];
      planeEquation[2] = normal[2];
      planeEquation[3] = -(planeEquation[0]*origin[0]+
        planeEquation[1]*origin[1]+
        planeEquation[2]*origin[2]);
      glClipPlane((GLenum)(GL_CLIP_PLANE0+i),planeEquation);
      }

    actorMatrix->Delete();  
    }

  this->Superclass::RenderInternal(renderer, actor, typeflags);

  for (i = 0; i < numClipPlanes; i++)
    {
    glDisable((GLenum)(GL_CLIP_PLANE0+i));
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLClipPlanesPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
