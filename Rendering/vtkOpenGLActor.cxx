/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>


#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLActor.h"
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "vtkObjectFactory.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
//-------------------------------------------------------------------------
vtkOpenGLActor* vtkOpenGLActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLActor");
  if(ret)
    {
    return (vtkOpenGLActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLActor;
}
#endif

// Actual actor render method.
void vtkOpenGLActor::Render(vtkRenderer *ren, vtkMapper *mapper)
{
  float opacity;

  // get opacity
  opacity = this->GetProperty()->GetOpacity();
  if (opacity == 1.0)
    {
    glDepthMask (GL_TRUE);
    }
  else
    {
    // add this check here for GL_SELECT mode
    // If we are not picking, then don't write to the zbuffer
    // because we probably haven't sorted the polygons. If we
    // are picking, then translucency doesn't matter - we want to
    // pick the thing closest to us.
    GLint param[1];
    glGetIntegerv(GL_RENDER_MODE, param);
    if(param[0] == GL_SELECT )
      {
      glDepthMask(GL_TRUE);
      }
    else
      {
      glDepthMask (GL_FALSE);
      }
    }

  // build transformation 
  double *mat = this->GetMatrix()->Element[0];
  double mat2[16];
  mat2[0] = mat[0];
  mat2[1] = mat[4];
  mat2[2] = mat[8];
  mat2[3] = mat[12];
  mat2[4] = mat[1];
  mat2[5] = mat[5];
  mat2[6] = mat[9];
  mat2[7] = mat[13];
  mat2[8] = mat[2];
  mat2[9] = mat[6];
  mat2[10] = mat[10];
  mat2[11] = mat[14];
  mat2[12] = mat[3];
  mat2[13] = mat[7];
  mat2[14] = mat[11];
  mat2[15] = mat[15];
  
  // insert model transformation 
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glMultMatrixd(mat2);

  // send a render to the mapper; update pipeline
  mapper->Render(ren,this);

  // pop transformation matrix
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  if (opacity != 1.0)
    {
    glDepthMask (GL_TRUE);
    }
}

