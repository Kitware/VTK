/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLight.cxx
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
#include "vtkOpenGLLight.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef VTK_USE_QUARTZ
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
//------------------------------------------------------------------------------
vtkOpenGLLight* vtkOpenGLLight::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLLight");
  if(ret)
    {
    return (vtkOpenGLLight*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLLight;
}
#endif



// Implement base class method.
void vtkOpenGLLight::Render(vtkRenderer *vtkNotUsed(ren),int light_index)
{
  float	dx, dy, dz;
  float	color[4];
  float Info[4];
  vtkMatrix4x4 *xform = NULL;

  // get required info from light
  color[0] = this->Intensity * this->Color[0];
  color[1] = this->Intensity * this->Color[1];
  color[2] = this->Intensity * this->Color[2];
  color[3] = 1.0;

  dx = this->FocalPoint[0] - this->Position[0];
  dy = this->FocalPoint[1] - this->Position[1];
  dz = this->FocalPoint[2] - this->Position[2];

  if(this->TransformMatrix != NULL) {
    xform = vtkMatrix4x4::New();
    xform->DeepCopy(this->TransformMatrix);
    xform->Transpose();

    // code assumes that we're already in GL_MODELVIEW matrix mode
    glPushMatrix();
    glMultMatrixd(xform->Element[0]);
  }

  glLightfv((GLenum)light_index, GL_DIFFUSE, color);
  glLightfv((GLenum)light_index, GL_SPECULAR, color);

  // define the light source
  if (!this->Positional)
    {
    Info[0]  = -dx;
    Info[1]  = -dy;
    Info[2]  = -dz;
    Info[3]  = 0.0;

    glLightf((GLenum)light_index, GL_SPOT_EXPONENT, 0);
    glLightf((GLenum)light_index, GL_SPOT_CUTOFF, 180);

    glLightfv((GLenum)light_index, GL_POSITION, Info );
    }
  else
    {
    // specify position and attenuation
    Info[0]  = this->Position[0];
    Info[1]  = this->Position[1];
    Info[2]  = this->Position[2];
    Info[3]  = 1.0;
    glLightfv((GLenum)light_index, GL_POSITION, Info );

    glLightf((GLenum)light_index, 
	     GL_CONSTANT_ATTENUATION, this->AttenuationValues[0]);
    glLightf((GLenum)light_index, 
	     GL_LINEAR_ATTENUATION, this->AttenuationValues[1]);
    glLightf((GLenum)light_index, 
	     GL_QUADRATIC_ATTENUATION, this->AttenuationValues[2]);

    // set up spot parameters if neccesary
    if (this->ConeAngle < 180.0)
      {
      Info[0] = dx;
      Info[1] = dy;
      Info[2] = dz;
      glLightfv((GLenum)light_index, GL_SPOT_DIRECTION, Info );
      glLightf((GLenum)light_index, GL_SPOT_EXPONENT, this->Exponent);
      glLightf((GLenum)light_index, GL_SPOT_CUTOFF, this->ConeAngle);
      }
    else
      {
      glLighti((GLenum)light_index, GL_SPOT_CUTOFF, 180);
      }
    }

  if(this->TransformMatrix != NULL) {
    glPopMatrix();
    xform->Delete();
  }
}

