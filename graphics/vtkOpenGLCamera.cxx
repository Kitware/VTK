/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLCamera.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
#include <GL/gl.h>
#endif
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"
#include "vtkgluPickMatrix.h"

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

// Implement base class method.
void vtkOpenGLCamera::Render(vtkRenderer *ren)
{
  float aspect[2];
  float *vport;
  int  *size, lowerLeft[2], upperRight[2];
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();

  // get the bounds of the window 
  size = (ren->GetRenderWindow())->GetSize();
  
  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  vport = ren->GetViewport();

  lowerLeft[0] = (int)(vport[0]*size[0] + 0.5);
  lowerLeft[1] = (int)(vport[1]*size[1] + 0.5);
  upperRight[0] = (int)(vport[2]*size[0] + 0.5);
  upperRight[1] = (int)(vport[3]*size[1] + 0.5);
  upperRight[0]--;
  upperRight[1]--;

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
  
  glViewport(lowerLeft[0],lowerLeft[1],
	     (upperRight[0]-lowerLeft[0]+1),
	     (upperRight[1]-lowerLeft[1]+1));
  glEnable( GL_SCISSOR_TEST );
  glScissor(lowerLeft[0],lowerLeft[1],
	    (upperRight[0]-lowerLeft[0]+1),
	    (upperRight[1]-lowerLeft[1]+1));
    
  /* for stereo we have to fiddle with aspect */
  aspect[0] = (float)(upperRight[0]-lowerLeft[0]+1)/
    (float)(upperRight[1]-lowerLeft[1]+1);
  aspect[1] = 1.0;
  
  ren->SetAspect(aspect);

  glMatrixMode( GL_PROJECTION);
  matrix->DeepCopy(this->GetPerspectiveTransformMatrix(aspect[0]/aspect[1],
						       -1,1));
  matrix->Transpose();
  if(ren->GetIsPicking())
    {
    GLint viewport[4];
    glGetIntegerv( GL_VIEWPORT, viewport);
    glLoadIdentity();
    vtkgluPickMatrix(ren->GetPickX(), ren->GetPickY(), 1, 1, viewport);
    glMultMatrixd(matrix->Element[0]);
    }
  else
    {
    // insert camera view transformation 
    glLoadMatrixd(matrix->Element[0]);
    }
  

  // since lookat modifies the model view matrix do a push 
  // first and set the mmode.  This will be undone in the  
  // render action after the actors! message sis sent      
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
