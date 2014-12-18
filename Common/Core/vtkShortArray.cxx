/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShortArray.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#include "vtkDataArrayTemplate.txx"
VTK_DATA_ARRAY_TEMPLATE_INSTANTIATE(short);
#include "vtkArrayIteratorTemplate.txx"
VTK_ARRAY_ITERATOR_TEMPLATE_INSTANTIATE(short);

#define vtkShortArray_cxx
#include "vtkShortArray.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkShortArray);

//----------------------------------------------------------------------------
vtkShortArray::vtkShortArray()
{
}

//----------------------------------------------------------------------------
vtkShortArray::~vtkShortArray()
{
}

//----------------------------------------------------------------------------
void vtkShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  this->RealSuperclass::PrintSelf(os,indent);
}
