/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlrLight.cxx
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
#include "vtkGlrRenderer.h"
#include "vtkGlrLight.h"

// Use directional light instead so mirror lights behave nicer 
static float light_info[] = {
  LCOLOR, 0.0, 0.0, 0.0,
  POSITION, 0.0, 0.0, 0.0, 0.0,
  LMNULL
  };


// Description:
// Implement base class method.
void vtkGlrLight::Render(vtkLight *lgt, vtkRenderer *ren,int light_index)
{
  this->Render(lgt, (vtkGlrRenderer *)ren,light_index);
}

// Description:
// Actual light render method.
void vtkGlrLight::Render(vtkLight *lgt, vtkGlrRenderer *vtkNotUsed(ren),
			 int light_index)
{
  float	dx, dy, dz;
  float	color[3];
  float *Color, *Position, *FocalPoint;
  float Intensity;

  // get required info from light
  Intensity = lgt->GetIntensity();
  Color = lgt->GetColor();
  color[0] = Intensity * Color[0];
  color[1] = Intensity * Color[1];
  color[2] = Intensity * Color[2];
  
  FocalPoint = lgt->GetFocalPoint();
  Position   = lgt->GetPosition();
  dx = FocalPoint[0] - Position[0];
  dy = FocalPoint[1] - Position[1];
  dz = FocalPoint[2] - Position[2];

  // define the light source
  light_info[1] = color[0];
  light_info[2] = color[1];
  light_info[3] = color[2];
  light_info[5] = -dx;
  light_info[6] = -dy;
  light_info[7] = -dz;
  
  vtkDebugMacro(<< "Defining light\n");
  lmdef(DEFLIGHT, light_index, 0, light_info);
}

