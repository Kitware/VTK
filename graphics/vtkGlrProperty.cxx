/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGlrProperty.cxx
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
#include "vtkGlrProperty.h"

// temporary material structure
static float mat[] = {
  ALPHA, 0.0,
  AMBIENT, 0.0, 0.0, 0.0,
  DIFFUSE, 0.0, 0.0, 0.0,
  SPECULAR, 0.0, 0.0, 0.0,
  SHININESS, 0.0,
  LMNULL
  };

// Description:
// Actual property render method.
void vtkGlrProperty::Render(vtkProperty *prop, vtkActor *vtkNotUsed(anActor),
			    vtkRenderer *vtkNotUsed(ren))
{
  int i, method;
  float Ambient, Diffuse, Specular;
  float *AmbientColor, *DiffuseColor, *SpecularColor;

  // unbind any textures for starters
  texbind(TX_TEXTURE_0,0);

  // turn on/off culling of surface primitives
  backface(prop->GetBackfaceCulling() ? TRUE : FALSE);
  frontface(prop->GetFrontfaceCulling() ? TRUE : FALSE);

  lmcolor (LMC_NULL);
  mat[1] = prop->GetOpacity();
  mat[15] = prop->GetSpecularPower();

  Ambient = prop->GetAmbient();
  Diffuse = prop->GetDiffuse();
  Specular = prop->GetSpecular();
  AmbientColor = prop->GetAmbientColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  for (i=0; i < 3; i++) 
    {
    mat[i+3] = Ambient*AmbientColor[i];
    mat[i+7] = Diffuse*DiffuseColor[i];
    mat[i+11] = Specular*SpecularColor[i];
    }

  lmdef(DEFMATERIAL, 1, 0, mat);
  lmbind(MATERIAL, 1);
  lmbind (BACKMATERIAL, 1);  

  // set interpolation 
  switch (prop->GetInterpolation()) 
    {
    case VTK_FLAT:
      method = FLAT;
      break;
    case VTK_GOURAUD:
    case VTK_PHONG:
      method = GOURAUD;
      break;
    default:
      method = GOURAUD;
      break;
    }
  
  shademodel(method);
}
