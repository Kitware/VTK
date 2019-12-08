/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSkybox.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSkybox.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkSkybox);

//------------------------------------------------------------------------------
void vtkSkybox::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
// Always return uninitialized
double* vtkSkybox::GetBounds()
{
  return nullptr;
}

//------------------------------------------------------------------------------
vtkSkybox::vtkSkybox()
{
  this->Projection = vtkSkybox::Cube;
  this->FloorPlane[0] = 0.0;
  this->FloorPlane[1] = 1.0;
  this->FloorPlane[2] = 0.0;
  this->FloorPlane[3] = 0.0;
  this->FloorRight[0] = 1.0;
  this->FloorRight[1] = 0.0;
  this->FloorRight[2] = 0.0;
}

//------------------------------------------------------------------------------
vtkSkybox::~vtkSkybox() = default;
