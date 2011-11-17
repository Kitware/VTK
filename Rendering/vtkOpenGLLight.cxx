/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLight.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLLight.h"

#include "vtkOpenGLRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkMatrix4x4.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif

#include <math.h>

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLLight);
#endif

// Implement base class method.
void vtkOpenGLLight::Render(vtkRenderer *vtkNotUsed(ren),int light_index)
{
  float dx, dy, dz;
  float color[4];
  float Info[4];

  // get required info from light

  dx = this->FocalPoint[0] - this->Position[0];
  dy = this->FocalPoint[1] - this->Position[1];
  dz = this->FocalPoint[2] - this->Position[2];

  if(this->TransformMatrix != NULL) 
    {
    double xform[16];
    vtkMatrix4x4::Transpose(*this->TransformMatrix->Element, xform);

    // code assumes that we're already in GL_MODELVIEW matrix mode
    glPushMatrix();
    glMultMatrixd(xform);
    }

  color[0] = this->Intensity * this->AmbientColor[0];
  color[1] = this->Intensity * this->AmbientColor[1];
  color[2] = this->Intensity * this->AmbientColor[2];
  color[3] = 1.0;
  glLightfv(static_cast<GLenum>(light_index), GL_AMBIENT, color);

  color[0] = this->Intensity * this->DiffuseColor[0];
  color[1] = this->Intensity * this->DiffuseColor[1];
  color[2] = this->Intensity * this->DiffuseColor[2];
  glLightfv(static_cast<GLenum>(light_index), GL_DIFFUSE, color);

  color[0] = this->Intensity * this->SpecularColor[0];
  color[1] = this->Intensity * this->SpecularColor[1];
  color[2] = this->Intensity * this->SpecularColor[2];
  glLightfv(static_cast<GLenum>(light_index), GL_SPECULAR, color);

  // define the light source
  if (!this->Positional)
    {
    Info[0]  = -dx;
    Info[1]  = -dy;
    Info[2]  = -dz;
    Info[3]  = 0.0;

    glLightf(static_cast<GLenum>(light_index), GL_SPOT_EXPONENT, 0);
    glLightf(static_cast<GLenum>(light_index), GL_SPOT_CUTOFF, 180);

    glLightfv(static_cast<GLenum>(light_index), GL_POSITION, Info );
    }
  else
    {
    // specify position and attenuation
    Info[0]  = this->Position[0];
    Info[1]  = this->Position[1];
    Info[2]  = this->Position[2];
    Info[3]  = 1.0;
    glLightfv(static_cast<GLenum>(light_index), GL_POSITION, Info );

    glLightf(static_cast<GLenum>(light_index), 
             GL_CONSTANT_ATTENUATION, this->AttenuationValues[0]);
    glLightf(static_cast<GLenum>(light_index), 
             GL_LINEAR_ATTENUATION, this->AttenuationValues[1]);
    glLightf(static_cast<GLenum>(light_index), 
             GL_QUADRATIC_ATTENUATION, this->AttenuationValues[2]);

    // set up spot parameters if neccesary
    if (this->ConeAngle < 180.0)
      {
      Info[0] = dx;
      Info[1] = dy;
      Info[2] = dz;
      glLightfv(static_cast<GLenum>(light_index), GL_SPOT_DIRECTION, Info );
      glLightf(static_cast<GLenum>(light_index), GL_SPOT_EXPONENT,
               this->Exponent);
      glLightf(static_cast<GLenum>(light_index), GL_SPOT_CUTOFF,
               this->ConeAngle);
      }
    else
      {
      glLighti(static_cast<GLenum>(light_index), GL_SPOT_CUTOFF, 180);
      }
    }

  if(this->TransformMatrix != NULL) 
    {
    glPopMatrix();
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLLight::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
