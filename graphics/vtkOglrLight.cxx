/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOglrLight.cxx
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
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

#include "vtkOglrRenderer.h"
#include "vtkOglrLight.h"

// Description:
// Implement base class method.
void vtkOglrLight::Render(vtkLight *lgt, vtkRenderer *ren,int light_index)
{
  this->Render(lgt, (vtkOglrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vtkOglrLight::Render(vtkLight *lgt, vtkOglrRenderer *vtkNotUsed(ren),
			  int light_index)
{
  float	dx, dy, dz;
  float	color[4];
  float *Color, *Position, *FocalPoint;
  float Intensity;
  float Info[4];

  // get required info from light
  Intensity = lgt->GetIntensity();
  Color = lgt->GetColor();
  color[0] = Intensity * Color[0];
  color[1] = Intensity * Color[1];
  color[2] = Intensity * Color[2];
  color[3] = 1.0;

  FocalPoint = lgt->GetFocalPoint();
  Position   = lgt->GetPosition();
  dx = FocalPoint[0] - Position[0];
  dy = FocalPoint[1] - Position[1];
  dz = FocalPoint[2] - Position[2];

  glLightfv((GLenum)light_index, GL_DIFFUSE, color);
  glLightfv((GLenum)light_index, GL_SPECULAR, color);

  // define the light source
  if (!lgt->GetPositional())
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
    Info[0]  = Position[0];
    Info[1]  = Position[1];
    Info[2]  = Position[2];
    Info[3]  = 1.0;
    glLightfv((GLenum)light_index, GL_POSITION, Info );

    float *AttenuationValues = lgt->GetAttenuationValues();
    glLightf((GLenum)light_index, 
	     GL_CONSTANT_ATTENUATION, AttenuationValues[0]);
    glLightf((GLenum)light_index, 
	     GL_LINEAR_ATTENUATION, AttenuationValues[1]);
    glLightf((GLenum)light_index, 
	     GL_QUADRATIC_ATTENUATION, AttenuationValues[2]);

    // set up spot parameters if neccesary
    if (lgt->GetConeAngle() < 180.0)
      {
      Info[0] = dx;
      Info[1] = dy;
      Info[2] = dz;
      glLightfv((GLenum)light_index, GL_SPOT_DIRECTION, Info );
      glLightf((GLenum)light_index, GL_SPOT_EXPONENT, lgt->GetExponent());
      glLightf((GLenum)light_index, GL_SPOT_CUTOFF, lgt->GetConeAngle());
      }
    else
      {
      glLighti((GLenum)light_index, GL_SPOT_CUTOFF, 180);
      }
    }

}

