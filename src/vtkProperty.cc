/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProperty.cc
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
#include <stdlib.h>
#include "vtkProperty.hh"
#include "vtkRenderer.hh"
#include "vtkRenderWindow.hh"
#include "vtkPropertyDevice.hh"
#include "vtkActor.hh"

// Description:
// Construct object with object color, ambient color, diffuse color,
// specular color, and edge color white; ambient coefficient=0; diffuse 
// coefficient=0; specular coefficient=0; specular power=1; Gouraud shading;
// and surface representation. Backface and frontface culling are off.
vtkProperty::vtkProperty()
{
  this->Color[0] = 1;
  this->Color[1] = 1;
  this->Color[2] = 1;

  this->AmbientColor[0] = 1;
  this->AmbientColor[1] = 1;
  this->AmbientColor[2] = 1;

  this->DiffuseColor[0] = 1;
  this->DiffuseColor[1] = 1;
  this->DiffuseColor[2] = 1;

  this->SpecularColor[0] = 1;
  this->SpecularColor[1] = 1;
  this->SpecularColor[2] = 1;

  this->EdgeColor[0] = 1;
  this->EdgeColor[1] = 1;
  this->EdgeColor[2] = 1;

  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->SpecularPower = 1.0;
  this->Opacity = 1.0;
  this->Interpolation = VTK_GOURAUD;
  this->Representation = VTK_SURFACE;
  this->EdgeVisibility = 0;
  this->Backface = 0;
  this->BackfaceCulling = 0;
  this->FrontfaceCulling = 0;

  this->Device = NULL;
}

vtkProperty::~vtkProperty()
{
  if (this->Device)
    {
    this->Device->Delete();
    }
}

// Description:
// Assign one property to another. 
vtkProperty& vtkProperty::operator=(const vtkProperty& p)
{
  this->Color[0] = p.Color[0];
  this->Color[1] = p.Color[1];
  this->Color[2] = p.Color[2];

  this->AmbientColor[0] = p.AmbientColor[0];
  this->AmbientColor[1] = p.AmbientColor[1];
  this->AmbientColor[2] = p.AmbientColor[2];

  this->DiffuseColor[0] = p.DiffuseColor[0];
  this->DiffuseColor[1] = p.DiffuseColor[1];
  this->DiffuseColor[2] = p.DiffuseColor[2];

  this->SpecularColor[0] = p.SpecularColor[0];
  this->SpecularColor[1] = p.SpecularColor[1];
  this->SpecularColor[2] = p.SpecularColor[2];

  this->EdgeColor[0] = p.EdgeColor[0];
  this->EdgeColor[1] = p.EdgeColor[1];
  this->EdgeColor[2] = p.EdgeColor[2];

  this->Ambient = p.Ambient;
  this->Diffuse = p.Diffuse;
  this->Specular = p.Specular;
  this->SpecularPower = p.SpecularPower;
  this->Opacity = p.Opacity;
  this->Interpolation = p.Interpolation;
  this->Representation = p.Representation;
  this->EdgeVisibility = p.EdgeVisibility;
  this->Backface = p.Backface;

  return *this;
}

void vtkProperty::Render(vtkRenderer *ren)
{
  if (!this->Device)
    {
    this->Device = ren->GetRenderWindow()->MakeProperty();
    }
  this->Device->Render(this,ren);
}

void vtkProperty::Render(vtkRenderer *ren, vtkActor *anActor)
{
  if (!this->Device)
    {
    this->Device = ren->GetRenderWindow()->MakeProperty();
    }
  this->Device->Render(this, anActor, ren);
}

void vtkProperty::SetFlat (void)
{
  this->Interpolation= VTK_FLAT;
}

void vtkProperty::SetGouraud (void)
{
  this->Interpolation = VTK_GOURAUD;
}

void vtkProperty::SetPhong (void)
{
  this->Interpolation = VTK_PHONG;
}

void vtkProperty::SetPoints (void)
{
  this->Interpolation = VTK_POINTS;
}

void vtkProperty::SetWireframe (void)
{
  this->Representation = VTK_WIREFRAME;
}

void vtkProperty::SetSurface (void)
{
  this->Representation = VTK_SURFACE;
}

void vtkProperty::SetColor(float R,float G,float B)
{
  /* store the coordinates */
  this->Color[0] = R;
  this->AmbientColor[0] = R;
  this->DiffuseColor[0] = R;
  this->SpecularColor[0] = R;

  this->Color[1] = G;
  this->AmbientColor[1] = G;
  this->DiffuseColor[1] = G;
  this->SpecularColor[1] = G;

  this->Color[2] = B;
  this->AmbientColor[2] = B;
  this->DiffuseColor[2] = B;
  this->SpecularColor[2] = B;
}

 
void vtkProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Ambient: " << this->Ambient << "\n";
  os << indent << "Ambient Color: (" << this->AmbientColor[0] << ", " 
    << this->AmbientColor[1] << ", " << this->AmbientColor[2] << ")\n";
  os << indent << "Backface: " << (this->Backface ? "On\n" : "Off\n");
  os << indent << "Color: (" << this->Color[0] << ", " 
    << this->Color[1] << ", " << this->Color[2] << ")\n";
  os << indent << "Diffuse: " << this->Diffuse << "\n";
  os << indent << "Diffuse Color: (" << this->DiffuseColor[0] << ", " 
    << this->DiffuseColor[1] << ", " << this->DiffuseColor[2] << ")\n";
  os << indent << "Edge Color: (" << this->EdgeColor[0] << ", " 
    << this->EdgeColor[1] << ", " << this->EdgeColor[2] << ")\n";
  os << indent << "Edge Visibility: " 
    << (this->EdgeVisibility ? "On\n" : "Off\n");
  os << indent << "Interpolation: ";
  switch (this->Interpolation) 
    {
    case 0: os << "VTK_FLAT\n"; break;
    case 1: os << "VTK_GOURAUD\n"; break;
    case 2: os << "VTK_PHONG\n"; break;
    default: os << "unknown\n";
    }
  os << indent << "Opacity: " << this->Opacity << "\n";
  os << indent << "Representation: ";
  switch (this->Representation) 
    {
    case 0: os << "VTK_POINTS\n"; break;
    case 1: os << "VTK_WIREFRAME\n"; break;
    case 2: os << "VTK_SURFACE\n"; break;
    default: os << "unknown\n";
    }
  os << indent << "Specular: " << this->Specular << "\n";
  os << indent << "Specular Color: (" << this->SpecularColor[0] << ", " 
     << this->SpecularColor[1] << ", " << this->SpecularColor[2] << ")\n";
  os << indent << "Specular Power: " << this->SpecularPower << "\n";
  os << indent << "Backface Culling: " 
     << (this->BackfaceCulling ? "On\n" : "Off\n");
  os << indent << "Frontface Culling: " 
     << (this->FrontfaceCulling ? "On\n" : "Off\n");
}
