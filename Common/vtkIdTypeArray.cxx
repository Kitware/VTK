/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIdTypeArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#include "vtkDataArrayTemplate.txx"
#if defined(VTK_USE_64BIT_IDS) // Only need this if vtkIdType is not int.
VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(vtkIdType);
#endif

#define __vtkIdTypeArray_cxx
#include "vtkIdTypeArray.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkIdTypeArray, "1.10");
vtkStandardNewMacro(vtkIdTypeArray);

//----------------------------------------------------------------------------
vtkIdTypeArray::vtkIdTypeArray(vtkIdType numComp): RealSuperclass(numComp)
{
}

//----------------------------------------------------------------------------
vtkIdTypeArray::~vtkIdTypeArray()
{
}

//----------------------------------------------------------------------------
void vtkIdTypeArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
