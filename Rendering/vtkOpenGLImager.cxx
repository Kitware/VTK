/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImager.cxx
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

#include "vtkOpenGLImager.h"
#include "vtkImageWindow.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
  #ifdef __APPLE__
    #include <OpenGL/gl.h>
  #else
    #include <GL/gl.h>
  #endif
#endif
#include "vtkObjectFactory.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
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
#endif



int vtkOpenGLImager::RenderOpaqueGeometry()
{
  int lowerLeft[2];

  float *vport = this->GetViewport();

  float vpu, vpv;
  vpu = vport[0];
  vpv = vport[1];  
  this->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = vport[2];
  vpv2 = vport[3];  
  this->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  // we will set this for all modes on the sparc
  glViewport(lowerLeft[0],lowerLeft[1],usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],usize,vsize);
  return vtkImager::RenderOpaqueGeometry();
}

void vtkOpenGLImager::Erase()
{
  int lowerLeft[2];

  float *vport = this->GetViewport();

  float vpu, vpv;
  vpu = vport[0];
  vpv = vport[1];  
  this->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = vport[2];
  vpv2 = vport[3];  
  this->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  // we will set this for all modes on the sparc
  glViewport(lowerLeft[0],lowerLeft[1],usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],usize,vsize);

  glClearDepth( (GLclampd)(1.0));
  glClearColor( ((GLclampf)(this->Background[0])),
                ((GLclampf)(this->Background[1])),
                ((GLclampf)(this->Background[2])),
                ((GLclampf)(1.0)) );

  vtkDebugMacro(<< "glClear\n");
  glClear((GLbitfield)GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

