/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSbrProperty.cc
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
#include "vtkSbrProperty.hh"
#include "vtkSbrRenderer.hh"
#include "starbase.c.h"

// Description:
// Implement base class method.
void vtkSbrProperty::Render(vtkProperty *prop, vtkRenderer *ren)
{
  this->Render(prop, (vtkSbrRenderer *)ren);
}

// Description:
// Actual property render method.
void vtkSbrProperty::Render(vtkProperty *prop, vtkSbrRenderer *ren)
{
  int fd;
  int     style;
  int     shininess;
  int     trans_level;
  static int pattern[16] = {0,10,8,2,5,15,13,7,4,14,12,6,1,11,9,3};
  gescape_arg esc_arg1,esc_arg2;
  int l;
  float *DiffuseColor, *EdgeColor, *SpecularColor;
  fd = ren->GetFd();
  
  EdgeColor = prop->GetEdgeColor();
  DiffuseColor = prop->GetDiffuseColor();
  SpecularColor = prop->GetSpecularColor();

  if (!prop->GetEdgeVisibility()) 
    {
    EdgeColor = DiffuseColor;
    }

  // turn on z buffering and disable/enable backface culling 
  if ( ! prop->GetBackfaceCulling() && ! prop->GetFrontfaceCulling() )
    {
    hidden_surface(fd, TRUE, FALSE);
    }
  else if ( prop->GetBackfaceCulling() )
    {
    hidden_surface(fd, TRUE, TRUE);
    }
  
  line_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  fill_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  perimeter_color(fd, EdgeColor[0], EdgeColor[1], EdgeColor[2]);
  text_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  marker_color(fd,DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);

  bf_fill_color(fd, DiffuseColor[0], DiffuseColor[1], DiffuseColor[2]);
  bf_perimeter_color(fd, EdgeColor[0], EdgeColor[1], EdgeColor[2]);

  switch (prop->GetRepresentation()) 
    {
  case VTK_POINTS:
    style = INT_POINT;
    break;
  case VTK_WIREFRAME:
    style = INT_OUTLINE;
    break;
  case VTK_SURFACE:
    style = INT_SOLID;
    break;
  default:
    style = INT_SOLID;
    break;
    }

  interior_style(fd, style, prop->GetEdgeVisibility());
  bf_interior_style(fd, style, prop->GetEdgeVisibility());
  surface_coefficients(fd, prop->GetAmbient(), prop->GetDiffuse(), 
		       prop->GetSpecular());
  bf_surface_coefficients(fd, prop->GetAmbient(), 
			  prop->GetDiffuse(), prop->GetSpecular());

  shininess = (int)prop->GetSpecularPower();
  if (shininess < 1) shininess = 1;
  if (shininess > 16383) shininess = 16383;
  
  surface_model(fd, TRUE, shininess, 
		SpecularColor[0], SpecularColor[1], SpecularColor[2]);
  bf_surface_model(fd, TRUE, shininess, 
		   SpecularColor[0], SpecularColor[1],
		   SpecularColor[2]);

  trans_level = (int)(16.0*(1.0 - prop->GetOpacity()));
  esc_arg1.i[0] = 0x0000;
  for (l = 0; l < trans_level; l++)
    esc_arg1.i[0] |= (0x0001 << pattern[l]);
  esc_arg1.i[0] = ~esc_arg1.i[0];
  gescape(fd, TRANSPARENCY, &esc_arg1, &esc_arg2);
}
