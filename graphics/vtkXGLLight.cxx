/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXGLLight.cxx
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
#include "vtkXGLRenderWindow.h"
#include "vtkXGLRenderer.h"
#include "vtkXGLLight.h"

// Description:
// Implement base class method.
void vtkXGLLight::Render(vtkRenderer *aren,int light_index)
{
  Xgl_light *lights;
  Xgl_color light_color;
  Xgl_pt_f3d direction;
  Xgl_pt_d3d position;
  vtkXGLRenderer *ren = (vtkXGLRenderer *)aren;
  
  // get required info from light
  light_color.rgb.r = this->Intensity * this->Color[0];
  light_color.rgb.g = this->Intensity * this->Color[1];
  light_color.rgb.b = this->Intensity * this->Color[2];
  
  direction.x = this->FocalPoint[0] - this->Position[0];
  direction.y = this->FocalPoint[1] - this->Position[1];
  direction.z = this->FocalPoint[2] - this->Position[2];
  position.x = this->Position[0];
  position.y = this->Position[1];
  position.z = this->Position[2];

  lights = ren->GetLightArray();

  if (this->Positional)
    {
    // XGL doesnt support second order attenuation so warn if non zero
    if (this->AttenuationValues[2] > 0.0)
      {
      vtkWarningMacro(<< "XGL doesn't support second order light attenuation!!!");
      }
    if (this->ConeAngle >= 180.0)
      {
      xgl_object_set(lights[light_index],
		     XGL_LIGHT_TYPE, XGL_LIGHT_POSITIONAL,
		     XGL_LIGHT_COLOR, &light_color,
		     XGL_LIGHT_POSITION, &position,
		     XGL_LIGHT_ATTENUATION_1, this->AttenuationValues[0],
		     XGL_LIGHT_ATTENUATION_2, this->AttenuationValues[1],
		     NULL);
      }
    else
      {
      // XGL spot angle is double what our convention is
      xgl_object_set(lights[light_index],
		     XGL_LIGHT_TYPE, XGL_LIGHT_SPOT,
		     XGL_LIGHT_COLOR, &light_color,
		     XGL_LIGHT_DIRECTION, &direction,
		     XGL_LIGHT_POSITION, &position,
		     XGL_LIGHT_SPOT_ANGLE, 
		     (this->ConeAngle*3.1415926/360.0),
		     XGL_LIGHT_SPOT_EXPONENT, this->Exponent,
		     XGL_LIGHT_ATTENUATION_1, this->AttenuationValues[0],
		     XGL_LIGHT_ATTENUATION_2, this->AttenuationValues[1],
		     NULL);
      }
    }
  else
    {
    // define the light source
    xgl_object_set(lights[light_index],
		   XGL_LIGHT_TYPE, XGL_LIGHT_DIRECTIONAL,
		   XGL_LIGHT_COLOR, &light_color,
		   XGL_LIGHT_DIRECTION, &direction,
		   NULL);
    }
  
  vtkDebugMacro(<< "Defining light\n");
}

