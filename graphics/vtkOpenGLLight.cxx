/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLight.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include <math.h>

#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLLight.h"
#include <GL/gl.h>

// Description:
// Implement base class method.
void vtkOpenGLLight::Render(vtkRenderer *vtkNotUsed(ren),int light_index)
{
  float	dx, dy, dz;
  float	color[4];
  float Info[4];

  // get required info from light
  color[0] = this->Intensity * this->Color[0];
  color[1] = this->Intensity * this->Color[1];
  color[2] = this->Intensity * this->Color[2];
  color[3] = 1.0;

  dx = this->FocalPoint[0] - this->Position[0];
  dy = this->FocalPoint[1] - this->Position[1];
  dz = this->FocalPoint[2] - this->Position[2];

  glLightfv((GLenum)light_index, GL_DIFFUSE, color);
  glLightfv((GLenum)light_index, GL_SPECULAR, color);

  // define the light source
  if (!this->Positional)
    {
    Info[0]  = -dx;
    Info[1]  = -dy;
    Info[2]  = -dz;
    Info[3]  = 0.0;
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

}

