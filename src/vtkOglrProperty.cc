/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOglrProperty.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkOglrRenderer.hh"
#include "vtkOglrProperty.hh"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>

// Description:
// Implement base class method.
void vtkOglrProperty::Render(vtkProperty *prop, vtkRenderer *ren)
{
  this->Render(prop, (vtkOglrRenderer *)ren);
}

// Description:
// Actual property render method.
void vtkOglrProperty::Render(vtkProperty *prop, vtkOglrRenderer *ren)
{
  int i;
  GLenum method;
  float Ambient, Diffuse, Specular;
  float *AmbientColor, *DiffuseColor, *SpecularColor;
  float Info[4];
  GLenum Face;

  // unbind any textures for starters
  glDisable(GL_TEXTURE_2D);

  glDisable(GL_COLOR_MATERIAL);

  Face = GL_FRONT_AND_BACK;
  // turn on/off backface culling
  if ( ! prop->GetBackfaceCulling() && ! prop->GetFrontfaceCulling() )
    {
    glDisable (GL_CULL_FACE);
    }
  else if ( prop->GetBackfaceCulling() )
    {
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    }
  else //if both front & back culling on, will fall into backface culling
    { //if you really want both front and back, use the Actor's visibility flag
    glCullFace (GL_FRONT);
    glEnable (GL_CULL_FACE);
    }

  Info[3] = prop->GetOpacity();

  // deal with blending if necc
  if (Info[3] < 1.0)
    {
    glEnable(GL_BLEND);
    }
  else
    {
    glDisable( GL_BLEND);
    }
  
  Ambient = prop->GetAmbient();
  Diffuse = prop->GetDiffuse();
  Specular = prop->GetSpecular();
  AmbientColor = prop->GetAmbientColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  for (i=0; i < 3; i++) 
    {
    Info[i] = Ambient*AmbientColor[i];
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = Diffuse*DiffuseColor[i];
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = Specular*SpecularColor[i];
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = prop->GetSpecularPower();
  glMaterialfv( Face, GL_SHININESS, Info );

  // set interpolation 
  switch (prop->GetInterpolation()) 
    {
    case VTK_FLAT:
      method = GL_FLAT;
      break;
    case VTK_GOURAUD:
    case VTK_PHONG:
      method = GL_SMOOTH;
      break;
    default:
      method = GL_SMOOTH;
      break;
    }
  
  glShadeModel(method);
}

