/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.cxx
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
#include "vtkOpenGLProperty.h"
#include <math.h>
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
//------------------------------------------------------------------------------
vtkOpenGLProperty* vtkOpenGLProperty::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLProperty");
  if(ret)
    {
    return (vtkOpenGLProperty*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLProperty;
}
#endif



// Implement base class method.
void vtkOpenGLProperty::Render(vtkActor *vtkNotUsed(anActor),
			     vtkRenderer *vtkNotUsed(ren))
{
  int i;
  GLenum method;
  float Info[4];
  GLenum Face;
  float  color[4];

  // unbind any textures for starters
  glDisable(GL_TEXTURE_2D);

  // disable alpha testing (this may have been enabled
  // by another actor in OpenGLTexture)
  glDisable (GL_ALPHA_TEST);

  glDisable(GL_COLOR_MATERIAL);

  Face = GL_FRONT_AND_BACK;
  // turn on/off backface culling
  if ( ! this->BackfaceCulling && ! this->FrontfaceCulling)
    {
    glDisable (GL_CULL_FACE);
    }
  else if ( this->BackfaceCulling)
    {
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    }
  else //if both front & back culling on, will fall into backface culling
    { //if you really want both front and back, use the Actor's visibility flag
    glCullFace (GL_FRONT);
    glEnable (GL_CULL_FACE);
    }

  Info[3] = this->Opacity;

  for (i=0; i < 3; i++) 
    {
    Info[i] = this->Ambient*this->AmbientColor[i];
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = this->Diffuse*this->DiffuseColor[i];
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = this->Specular*this->SpecularColor[i];
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = this->SpecularPower;
  glMaterialfv( Face, GL_SHININESS, Info );

  // set interpolation 
  switch (this->Interpolation) 
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

  // The material properties set above are used if shading is
  // enabled. This color set here is used if shading is 
  // disabled. Shading is disabled in the 
  // vtkOpenGLPolyDataMapper::Draw() method if points or lines
  // are encountered without normals. 
  this->GetColor( color );
  color[3] = 1.0;
  glColor4fv( color );

  // Set the PointSize
  glPointSize (this->PointSize);

  // Set the LineWidth
  glLineWidth (this->LineWidth);

  // Set the LineStipple
  if (this->LineStipplePattern != 0xFFFF)
    {
    glEnable (GL_LINE_STIPPLE);
    glLineStipple (this->LineStippleRepeatFactor, this->LineStipplePattern);
    }
  else
    {
    glDisable (GL_LINE_STIPPLE);
    }
}

// Implement base class method.
void vtkOpenGLProperty::BackfaceRender(vtkActor *vtkNotUsed(anActor),
			     vtkRenderer *vtkNotUsed(ren))
{
  int i;
  float Info[4];
  GLenum Face;

  Face = GL_BACK;

  Info[3] = this->Opacity;

  for (i=0; i < 3; i++) 
    {
    Info[i] = this->Ambient*this->AmbientColor[i];
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = this->Diffuse*this->DiffuseColor[i];
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = this->Specular*this->SpecularColor[i];
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = this->SpecularPower;
  glMaterialfv( Face, GL_SHININESS, Info );

}

