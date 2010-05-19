/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsignedShortArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#include "vtkDataArrayTemplate.txx"
VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned short);
#include "vtkArrayIteratorTemplate.txx"
VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(unsigned short);

#define __vtkUnsignedShortArray_cxx
#include "vtkUnsignedShortArray.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUnsignedShortArray);

//----------------------------------------------------------------------------
vtkUnsignedShortArray::vtkUnsignedShortArray(vtkIdType numComp): RealSuperclass(numComp)
{
}

//----------------------------------------------------------------------------
vtkUnsignedShortArray::~vtkUnsignedShortArray()
{
}

//----------------------------------------------------------------------------
void vtkUnsignedShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
