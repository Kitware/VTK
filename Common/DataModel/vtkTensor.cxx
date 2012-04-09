/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTensor.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkTensor);

// Construct tensor initially pointing to internal storage.
vtkTensor::vtkTensor()
{
  this->T = this->Storage;
  for (int j=0; j<3; j++)
    {
    for (int i=0; i<3; i++)
      {
      this->T[i+j*3] = 0.0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkTensor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  for (int j=0; j<3; j++)
    {
    os << indent;
    for (int i=0; i<3; i++)
      {
      os << this->Storage[i+j*3] << " ";
      }
    os << "\n";
    }
}
