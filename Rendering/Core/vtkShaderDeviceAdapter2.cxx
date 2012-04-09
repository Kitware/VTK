/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderDeviceAdapter2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkShaderDeviceAdapter2.h"

#include "vtkObjectFactory.h"
#include "vtkShaderProgram.h"

//---------------------------------------------------------------------------
vtkShaderDeviceAdapter2::vtkShaderDeviceAdapter2()
{
  this->ShaderProgram = 0;
}

//---------------------------------------------------------------------------
vtkShaderDeviceAdapter2::~vtkShaderDeviceAdapter2()
{
  this->SetShaderProgram(0);
}

//---------------------------------------------------------------------------
void vtkShaderDeviceAdapter2::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ShaderProgram: " << this->ShaderProgram << endl;
}
