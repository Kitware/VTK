/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnsigned__Int64Array.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#include "vtkDataArrayTemplate.txx"
VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(unsigned __int64);
#include "vtkArrayIteratorTemplate.txx"
VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(unsigned __int64);

#define vtkUnsigned__Int64Array_cxx
#include "vtkUnsigned__Int64Array.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkUnsigned__Int64Array);

//----------------------------------------------------------------------------
vtkUnsigned__Int64Array::vtkUnsigned__Int64Array(vtkIdType numComp):
  RealSuperclass(numComp)
{
}

//----------------------------------------------------------------------------
vtkUnsigned__Int64Array::~vtkUnsigned__Int64Array()
{
}

//----------------------------------------------------------------------------
void vtkUnsigned__Int64Array::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
