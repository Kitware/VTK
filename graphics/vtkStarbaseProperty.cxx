/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStarbaseProperty.cxx
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
#include "vtkStarbaseProperty.h"
#include "vtkStarbaseRenderer.h"
#include "starbase.c.h"

// Description:
// Implement base class method.
void vtkStarbaseProperty::Render(vtkActor *vtkNotUsed(anAct), vtkRenderer *aren)
{
  vtkStarbaseRenderer *ren = (vtkStarbaseRenderer *)aren;
  int fd;
  int     style;
  int     shininess;
  int     trans_level;
  static int pattern[16] = {0,10,8,2,5,15,13,7,4,14,12,6,1,11,9,3};
  gescape_arg esc_arg1,esc_arg2;
  int l;
  fd = ren->GetFd();
  
  if (!this->EdgeVisibility) 
    {
    EdgeColor = DiffuseColor;
    }

  // turn on z buffering and disable/enable backface culling 
  if ( ! this->BackfaceCulling && !this->FrontfaceCulling)
    {
    hidden_surface(fd, TRUE, FALSE);
    }
  else if (this->BackfaceCulling)
    {
    hidden_surface(fd, TRUE, TRUE);
    }
  
  line_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
	     this->DiffuseColor[2]);
  fill_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
	     this->DiffuseColor[2]);
  perimeter_color(fd, this->EdgeColor[0], this->EdgeColor[1], 
		  this->EdgeColor[2]);
  text_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
	     this->DiffuseColor[2]);
  marker_color(fd,this->DiffuseColor[0], this->DiffuseColor[1], 
	       this->DiffuseColor[2]);

  bf_fill_color(fd, this->DiffuseColor[0], this->DiffuseColor[1], 
		this->DiffuseColor[2]);
  bf_perimeter_color(fd, this->EdgeColor[0], this->EdgeColor[1], 
		     this->EdgeColor[2]);

  switch (this->Representation) 
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

  interior_style(fd, style, this->EdgeVisibility);
  bf_interior_style(fd, style, this->EdgeVisibility);
  surface_coefficients(fd, this->Ambient, this->Diffuse, this->Specular);
  bf_surface_coefficients(fd, this->Ambient, this->Diffuse, this->Specular);

  shininess = (int)this->SpecularPower;
  if (shininess < 1) shininess = 1;
  if (shininess > 16383) shininess = 16383;
  
  surface_model(fd, TRUE, shininess, 
		this->SpecularColor[0], this->SpecularColor[1], 
		this->SpecularColor[2]);
  bf_surface_model(fd, TRUE, shininess, 
		   this->SpecularColor[0], this->SpecularColor[1],
		   this->SpecularColor[2]);

  trans_level = (int)(16.0*(1.0 - this->Opacity));
  esc_arg1.i[0] = 0x0000;
  for (l = 0; l < trans_level; l++)
    esc_arg1.i[0] |= (0x0001 << pattern[l]);
  esc_arg1.i[0] = ~esc_arg1.i[0];
  gescape(fd, TRANSPARENCY, &esc_arg1, &esc_arg2);
}
