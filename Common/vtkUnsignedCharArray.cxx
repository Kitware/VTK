/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedCharArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#include "vtkDataArrayTemplate.txx"
VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned char);

#define __vtkUnsignedCharArray_cxx
#include "vtkUnsignedCharArray.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkUnsignedCharArray, "1.59");
vtkStandardNewMacro(vtkUnsignedCharArray);

//----------------------------------------------------------------------------
vtkUnsignedCharArray::vtkUnsignedCharArray(vtkIdType numComp): RealSuperclass(numComp)
{
}

//----------------------------------------------------------------------------
vtkUnsignedCharArray::~vtkUnsignedCharArray()
{
}

//----------------------------------------------------------------------------
void vtkUnsignedCharArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
