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
      vtkErrorMacro(<< "OpenGL guarantees at most 6 additional clipping planes");
      }
    }

  for (i = 0; i < numClipPlanes; i++)
    {
    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0+i));
    }

  if ( clipPlanes )
    {
    vtkMatrix4x4 *actorMatrix = vtkMatrix4x4::New();
    actor->GetMatrix( actorMatrix );
    actorMatrix->Invert();
    // OpenGL matrices are transposed compared to VTK matrices
    actorMatrix->Transpose();

    double origin[4], normal[3];

    for (i = 0; i < numClipPlanes; i++)
      {    
      plane = static_cast<vtkPlane *>(clipPlanes->GetItemAsObject(i));

      plane->GetOrigin(origin);
      plane->GetNormal(normal);

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glMultMatrixd(actorMatrix->Element[0]);

      planeEquation[0] = normal[0];
      planeEquation[1] = normal[1];
      planeEquation[2] = normal[2];
      planeEquation[3] = -(planeEquation[0]*origin[0]+
        planeEquation[1]*origin[1]+
        planeEquation[2]*origin[2]);
      glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0+i),planeEquation);

      glPopMatrix();
      }

    actorMatrix->Delete();  
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
