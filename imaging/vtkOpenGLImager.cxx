/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImager.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkOpenGLImager.h"
#include "vtkImageWindow.h"
#include <GL/gl.h>
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkOpenGLImager* vtkOpenGLImager::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLImager");
  if(ret)
    {
    return (vtkOpenGLImager*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLImager;
}




int vtkOpenGLImager::RenderOpaqueGeometry()
{
  int *size, lowerLeft[2], upperRight[2];

  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();

  // determine the inclusive bounds of the viewport
  // then find the corresponding pixel 
  lowerLeft[0] = (int)(this->Viewport[0]*size[0] + 0.5);
  lowerLeft[1] = (int)(this->Viewport[1]*size[1] + 0.5);
  upperRight[0] = (int)(this->Viewport[2]*size[0] + 0.5);
  upperRight[1] = (int)(this->Viewport[3]*size[1] + 0.5);
  upperRight[0]--;
  upperRight[1]--;

  // we will set this for all modes on the sparc
  glViewport(lowerLeft[0],lowerLeft[1],
	     (upperRight[0]-lowerLeft[0]+1),
	     (upperRight[1]-lowerLeft[1]+1));
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],
	    (upperRight[0]-lowerLeft[0]+1),
	    (upperRight[1]-lowerLeft[1]+1));
  return vtkImager::RenderOpaqueGeometry();
}

void vtkOpenGLImager::Erase()
{
  int *size, lowerLeft[2], upperRight[2];

  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();

  // determine the inclusive bounds of the viewport
  // then find the corresponding pixel 
  lowerLeft[0] = (int)(this->Viewport[0]*size[0] + 0.5);
  lowerLeft[1] = (int)(this->Viewport[1]*size[1] + 0.5);
  upperRight[0] = (int)(this->Viewport[2]*size[0] + 0.5);
  upperRight[1] = (int)(this->Viewport[3]*size[1] + 0.5);
  upperRight[0]--;
  upperRight[1]--;

  // we will set this for all modes on the sparc
  glViewport(lowerLeft[0],lowerLeft[1],
	     (upperRight[0]-lowerLeft[0]+1),
	     (upperRight[1]-lowerLeft[1]+1));
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],
	    (upperRight[0]-lowerLeft[0]+1),
	    (upperRight[1]-lowerLeft[1]+1));

  glClearColor( ((GLclampf)(this->Background[0])),
                ((GLclampf)(this->Background[1])),
                ((GLclampf)(this->Background[2])),
                ((GLclampf)(1.0)) );

  vtkDebugMacro(<< "glClear\n");
  glClear((GLbitfield)GL_COLOR_BUFFER_BIT);
}

