/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXGLProperty.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkXGLProperty.h"

// Description:
// Implement base class method.
void vtkXGLProperty::Render(vtkActor *vtkNotUsed(anAct),
			     vtkRenderer *aren)
{
  vtkXGLRenderer *ren = (vtkXGLRenderer *)aren;
  int method, line_method, twoSidedLighting;
  Xgl_ctx *context;
  Xgl_color_rgb diffuseColor;
  Xgl_color_rgb specularColor;

  // get the context for this renderer 
  context = ren->GetContext();
  diffuseColor.r = this->DiffuseColor[0];
  diffuseColor.g = this->DiffuseColor[1];
  diffuseColor.b = this->DiffuseColor[2];
  specularColor.r = this->SpecularColor[0];
  specularColor.g = this->SpecularColor[1];
  specularColor.b = this->SpecularColor[2];

  if ( ! this->BackfaceCulling && ! this->FrontfaceCulling)
    {
    xgl_object_set(*context, XGL_3D_CTX_SURF_FACE_CULL, XGL_CULL_OFF, 0);
    }
  else if (this->BackfaceCulling)
    {
    xgl_object_set(*context, XGL_3D_CTX_SURF_FACE_CULL, XGL_CULL_BACK, 0);
    }
  else //if both front & back culling on, will fall into backface culling
    { //if you really want both front and back, use the Actor's visibility flag
    xgl_object_set(*context, XGL_3D_CTX_SURF_FACE_CULL, XGL_CULL_FRONT, 0);
    }

  // set property according to backface and two-sided lighting flags
  twoSidedLighting = ren->GetTwoSidedLighting();

  if ( ! this->Backface && twoSidedLighting ) 
    {
    xgl_object_set(*context,
               XGL_3D_CTX_SURF_FRONT_AMBIENT, this->Ambient,
               XGL_3D_CTX_SURF_FRONT_DIFFUSE, this->Diffuse,
               XGL_3D_CTX_SURF_FRONT_SPECULAR, this->Specular,
               XGL_3D_CTX_SURF_FRONT_SPECULAR_POWER, this->SpecularPower,
               XGL_3D_CTX_SURF_FRONT_SPECULAR_COLOR, &specularColor,
               XGL_CTX_SURF_FRONT_COLOR, &diffuseColor,
               XGL_3D_CTX_SURF_FRONT_TRANSP, 1.0-this->Opacity,
               XGL_CTX_LINE_COLOR, &diffuseColor,
               XGL_3D_CTX_SURF_BACK_AMBIENT, this->Ambient,
               XGL_3D_CTX_SURF_BACK_DIFFUSE, this->Diffuse,
               XGL_3D_CTX_SURF_BACK_SPECULAR, this->Specular,
               XGL_3D_CTX_SURF_BACK_SPECULAR_POWER, this->SpecularPower,
               XGL_3D_CTX_SURF_BACK_SPECULAR_COLOR, &specularColor,
               XGL_3D_CTX_SURF_BACK_COLOR, &diffuseColor,
               XGL_3D_CTX_SURF_BACK_TRANSP, 1.0-this->Opacity,
               NULL);
    }
  else if ( ! this->Backface && ! twoSidedLighting )
    {
    static float bfaceAmbient=0.0;
    static float bfaceDiffuse=0.0;
    static float bfaceSpecular=0.0;
    static float bfaceSpecularPower=0.0;

    xgl_object_set(*context,
               XGL_3D_CTX_SURF_FRONT_AMBIENT, this->Ambient,
               XGL_3D_CTX_SURF_FRONT_DIFFUSE, this->Diffuse,
               XGL_3D_CTX_SURF_FRONT_SPECULAR, this->Specular,
               XGL_3D_CTX_SURF_FRONT_SPECULAR_POWER, this->SpecularPower,
               XGL_3D_CTX_SURF_FRONT_SPECULAR_COLOR, &specularColor,
               XGL_CTX_SURF_FRONT_COLOR, &diffuseColor,
               XGL_3D_CTX_SURF_FRONT_TRANSP, 1.0-this->Opacity,
               XGL_CTX_LINE_COLOR, &diffuseColor,
               XGL_3D_CTX_SURF_BACK_AMBIENT, bfaceAmbient,
               XGL_3D_CTX_SURF_BACK_DIFFUSE, bfaceDiffuse,
               XGL_3D_CTX_SURF_BACK_SPECULAR, bfaceSpecular,
               XGL_3D_CTX_SURF_BACK_SPECULAR_POWER, bfaceSpecularPower,
               XGL_3D_CTX_SURF_BACK_SPECULAR_COLOR, &specularColor,
               XGL_3D_CTX_SURF_BACK_COLOR, &diffuseColor,
               XGL_3D_CTX_SURF_BACK_TRANSP, 1.0-this->Opacity,
               NULL);
    }
  else 
    {
    vtkErrorMacro(<<"Backface properties not implemented yet");
    return;
    }		 

  switch (this->Representation) 
    {
    case VTK_POINTS:
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     NULL);
      break;
    case VTK_WIREFRAME:
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_HOLLOW,
		     NULL);
      break;
    case VTK_SURFACE:
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     NULL);
      break;
    default: 
      xgl_object_set(*context,
		     XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_SOLID,
		     NULL);
      break;
    }

  // set interpolation 
  switch (this->Interpolation) 
    {
    case VTK_FLAT:
      method = XGL_ILLUM_PER_FACET;
      line_method = FALSE;
      break;
    case VTK_GOURAUD:
    case VTK_PHONG:
      method = XGL_ILLUM_PER_VERTEX;
      line_method = TRUE;
      break;
    default:
      method = XGL_ILLUM_PER_VERTEX;
      line_method = TRUE;
      break;
    }
  xgl_object_set(*context,
		 XGL_3D_CTX_SURF_FRONT_ILLUMINATION, method,
		 XGL_3D_CTX_SURF_BACK_ILLUMINATION, method,
		 XGL_3D_CTX_LINE_COLOR_INTERP, line_method,
		 NULL);
}

// Description:
// Implement base class method.


void vtkXGLProperty::BackfaceRender(vtkActor *vtkNotUsed(anAct),
			     vtkRenderer *aren)
{
  vtkXGLRenderer *ren = (vtkXGLRenderer *)aren;
  Xgl_ctx *context;
  Xgl_color_rgb diffuseColor;
  Xgl_color_rgb specularColor;

  // get the context for this renderer 
  context = ren->GetContext();
  diffuseColor.r = this->DiffuseColor[0];
  diffuseColor.g = this->DiffuseColor[1];
  diffuseColor.b = this->DiffuseColor[2];
  specularColor.r = this->SpecularColor[0];
  specularColor.g = this->SpecularColor[1];
  specularColor.b = this->SpecularColor[2];

  xgl_object_set(*context,
             XGL_3D_CTX_SURF_BACK_AMBIENT, this->Ambient,
             XGL_3D_CTX_SURF_BACK_DIFFUSE, this->Diffuse,
             XGL_3D_CTX_SURF_BACK_SPECULAR, this->Specular,
             XGL_3D_CTX_SURF_BACK_SPECULAR_POWER, this->SpecularPower,
             XGL_3D_CTX_SURF_BACK_SPECULAR_COLOR, &specularColor,
             XGL_3D_CTX_SURF_BACK_COLOR, &diffuseColor,
             XGL_3D_CTX_SURF_BACK_TRANSP, 1.0-this->Opacity,
             NULL);
}
