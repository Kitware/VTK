/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.cxx
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

#include "vtkRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLCamera.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkgluPickMatrix.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
//------------------------------------------------------------------------------
vtkOpenGLCamera* vtkOpenGLCamera::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLCamera");
  if(ret)
    {
    return (vtkOpenGLCamera*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLCamera;
}
#endif

// Implement base class method.
void vtkOpenGLCamera::Render(vtkRenderer *ren)
{
  float aspect[2];
  float *vport;
  int  lowerLeft[2];
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  vport = ren->GetViewport();

  float vpu, vpv;
  vpu = vport[0];
  vpv = vport[1];  
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = vport[2];
  vpv2 = vport[3];  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  // if were on a stereo renderer draw to special parts of screen
  if (this->Stereo)
    {
    switch ((ren->GetRenderWindow())->GetStereoType())
      {
      case VTK_STEREO_CRYSTAL_EYES:
        if (this->LeftEye)
          {
          glDrawBuffer(GL_BACK_LEFT);
          }
        else
          {
          glDrawBuffer(GL_BACK_RIGHT);
          }
        break;
      case VTK_STEREO_LEFT:
	this->LeftEye = 1;
	break;
      case VTK_STEREO_RIGHT:
	this->LeftEye = 0;
	break;
      default:
        break;
      }
    }
  else
    {
    if (ren->GetRenderWindow()->GetDoubleBuffer())
      {
      glDrawBuffer(GL_BACK);
      }
    else
      {
      glDrawBuffer(GL_FRONT);
      }
    }
  
  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);
    
  ren->ComputeAspect();
  ren->GetAspect(aspect);

  glMatrixMode( GL_PROJECTION);
  matrix->DeepCopy(this->GetPerspectiveTransformMatrix(aspect[0]/aspect[1],
						       -1,1));
  matrix->Transpose();
  if(ren->GetIsPicking())
    {
    int size[2]; size[0] = usize; size[1] = vsize;
    glLoadIdentity();
    vtkgluPickMatrix(ren->GetPickX(), ren->GetPickY(), 1, 1, lowerLeft, size);
    glMultMatrixd(matrix->Element[0]);
    }
  else
    {
    // insert camera view transformation 
    glLoadMatrixd(matrix->Element[0]);
    }
  
  // push the model view matrix onto the stack, make sure we 
  // adjust the mode first
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  matrix->DeepCopy(this->GetViewTransformMatrix());
  matrix->Transpose();
  
  // insert camera view transformation 
  glMultMatrixd(matrix->Element[0]);

  if ((ren->GetRenderWindow())->GetErase())
    {
    ren->Clear();
    }

  // if we have a stereo renderer, draw other eye next time 
  if (this->Stereo)
    {
    if (this->LeftEye)
      {
      this->LeftEye = 0;
      }
    else
      {
      this->LeftEye = 1;
      }
    }

  matrix->Delete();
}

void vtkOpenGLCamera::UpdateViewport(vtkRenderer *ren)
{
  float *vport;
  int  lowerLeft[2];

  vport = ren->GetViewport();

  float vpu, vpv;
  vpu = vport[0];
  vpv = vport[1];  
  ren->NormalizedDisplayToDisplay(vpu,vpv);
  lowerLeft[0] = (int)(vpu+0.5);
  lowerLeft[1] = (int)(vpv+0.5);
  float vpu2, vpv2;
  vpu2 = vport[2];
  vpv2 = vport[3];  
  ren->NormalizedDisplayToDisplay(vpu2,vpv2);
  int usize = (int)(vpu2 + 0.5) - lowerLeft[0];
  int vsize = (int)(vpv2 + 0.5) - lowerLeft[1];  

  glViewport(lowerLeft[0],lowerLeft[1], usize, vsize);
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1], usize, vsize);
}
