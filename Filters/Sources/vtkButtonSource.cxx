/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkButtonSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkButtonSource.h"


// Construct
vtkButtonSource::vtkButtonSource()
{
  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->ShoulderTextureCoordinate[0] = 0.0;
  this->ShoulderTextureCoordinate[1] = 0.0;
  this->TextureStyle = VTK_TEXTURE_STYLE_PROPORTIONAL;
  this->TextureDimensions[0] = 100;
  this->TextureDimensions[1] = 100;
  this->TwoSided = 0;

  this->SetNumberOfInputPorts(0);
}

void vtkButtonSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";

  os << indent << "Shoulder Texture Coordinate: ("
     << this->ShoulderTextureCoordinate[0] << ", "
     << this->ShoulderTextureCoordinate[1] << ")\n";

  os << indent << "Texture Style: ";
  if ( this->TextureStyle == VTK_TEXTURE_STYLE_FIT_IMAGE )
    {
    os << "Fit\n";
    }
  else
    {
    os << "Proportional\n";
    }

  os << indent << "Texture Dimensions: ("
     << this->TextureDimensions[0] << ", "
     << this->TextureDimensions[1] << ")\n";

  os << indent << "Two Sided: "
     << (this->TwoSided ? "On\n" : "Off\n");
}
