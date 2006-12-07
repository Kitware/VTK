/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLProperty.h"
#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkToolkits.h"  // for VTK_USE_GL2PS

#ifdef VTK_USE_GL2PS
#include "gl2ps.h"
#include "vtkGL2PSExporter.h"
#endif // VTK_USE_GL2PS


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLProperty, "1.34");
vtkStandardNewMacro(vtkOpenGLProperty);
#endif



// Implement base class method.
void vtkOpenGLProperty::Render(vtkActor *anActor,
                             vtkRenderer *ren)
{
  int i;
  GLenum method;
  float Info[4];
  GLenum Face;
  double  color[4];

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
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
  else if ( this->BackfaceCulling)
    {
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    if (this->GetRepresentation() == VTK_WIREFRAME)
      {
      glPolygonMode(GL_FRONT, GL_LINE); 
      }
    else if (this->GetRepresentation() == VTK_SURFACE)
      {
      glPolygonMode(GL_FRONT, GL_FILL);
      }
    else
      {
      glPolygonMode(GL_FRONT, GL_POINT);
      }
    }
  else //if both front & back culling on, will fall into backface culling
    { //if you really want both front and back, use the Actor's visibility flag
    glCullFace (GL_FRONT);
    glEnable (GL_CULL_FACE);
    if (this->GetRepresentation() == VTK_WIREFRAME)
      {
      glPolygonMode(GL_BACK, GL_LINE);
      }  
    else if (this->GetRepresentation() == VTK_SURFACE)
      {
      glPolygonMode(GL_BACK, GL_FILL);
      }
    else
      {
      glPolygonMode(GL_BACK, GL_POINT);
      }
    }

  Info[3] = this->Opacity;

  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(this->Ambient*this->AmbientColor[i]);
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(this->Diffuse*this->DiffuseColor[i]);
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(this->Specular*this->SpecularColor[i]);
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = static_cast<float>(this->SpecularPower);
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
  color[3] = this->Opacity;
  glColor4dv( color );

  // Set the PointSize
  glPointSize (this->PointSize);

  // Set the LineWidth
  glLineWidth (this->LineWidth);

  // Set pointsize and linewidth for GL2PS output.
#ifdef VTK_USE_GL2PS
  gl2psPointSize(this->PointSize*
                 vtkGL2PSExporter::GetGlobalPointSizeFactor());
  gl2psLineWidth(this->LineWidth*
                 vtkGL2PSExporter::GetGlobalLineWidthFactor());
#endif // VTK_USE_GL2PS

  // Set the LineStipple
  if (this->LineStipplePattern != 0xFFFF)
    {
    glEnable (GL_LINE_STIPPLE);
#ifdef VTK_USE_GL2PS
    gl2psEnable(GL2PS_LINE_STIPPLE);
#endif // VTK_USE_GL2PS
    glLineStipple (this->LineStippleRepeatFactor, this->LineStipplePattern);
    }
  else
    {
    glDisable (GL_LINE_STIPPLE);
#ifdef VTK_USE_GL2PS
    gl2psDisable(GL2PS_LINE_STIPPLE);
#endif // VTK_USE_GL2PS
    }

  this->Superclass::Render(anActor, ren);
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
    Info[i] = static_cast<float>(this->Ambient*this->AmbientColor[i]);
    }
  glMaterialfv( Face, GL_AMBIENT, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(this->Diffuse*this->DiffuseColor[i]);
    }
  glMaterialfv( Face, GL_DIFFUSE, Info );
  for (i=0; i < 3; i++) 
    {
    Info[i] = static_cast<float>(this->Specular*this->SpecularColor[i]);
    }
  glMaterialfv( Face, GL_SPECULAR, Info );

  Info[0] = static_cast<float>(this->SpecularPower);
  glMaterialfv( Face, GL_SHININESS, Info );

}

//----------------------------------------------------------------------------
void vtkOpenGLProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
