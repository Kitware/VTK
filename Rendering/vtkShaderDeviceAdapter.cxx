/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderDeviceAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShaderDeviceAdapter.h"

#include "vtkObjectFactory.h"
#include "vtkShaderProgram.h"


//---------------------------------------------------------------------------
vtkShaderDeviceAdapter::vtkShaderDeviceAdapter()
{
  this->ShaderProgram = 0;
}

//---------------------------------------------------------------------------
vtkShaderDeviceAdapter::~vtkShaderDeviceAdapter()
{
  this->SetShaderProgram(0);
}

//---------------------------------------------------------------------------
void vtkShaderDeviceAdapter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ShaderProgram: " << this->ShaderProgram << endl;
}
