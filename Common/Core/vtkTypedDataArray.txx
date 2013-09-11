/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTypedDataArray.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkTypedDataArray_txx
#define __vtkTypedDataArray_txx

#include "vtkTypedDataArray.h"

//------------------------------------------------------------------------------
template <typename Scalar>
vtkTypedDataArray<Scalar>::vtkTypedDataArray()
{
}

//------------------------------------------------------------------------------
template <typename Scalar>
vtkTypedDataArray<Scalar>::~vtkTypedDataArray()
{
}

//------------------------------------------------------------------------------
template <typename Scalar> inline
int vtkTypedDataArray<Scalar>::GetDataType()
{
  return vtkTypeTraits<Scalar>::VTK_TYPE_ID;
}

//------------------------------------------------------------------------------
template <typename Scalar> inline
int vtkTypedDataArray<Scalar>::GetDataTypeSize()
{
  return static_cast<int>(sizeof(Scalar));
}

//------------------------------------------------------------------------------
template <typename Scalar> inline
void vtkTypedDataArray<Scalar>::SetNumberOfValues(vtkIdType number)
{
  if (this->Allocate(number))
    {
    this->MaxId = number - 1;
    }
  this->Modified();
}

//------------------------------------------------------------------------------
template <typename Scalar> inline vtkTypedDataArray<Scalar> *
vtkTypedDataArray<Scalar>::FastDownCast(vtkAbstractArray *source)
{
  switch (source->GetArrayType())
    {
    case vtkAbstractArray::DataArrayTemplate:
    case vtkAbstractArray::TypedDataArray:
    case vtkAbstractArray::MappedDataArray:
      if (source->GetDataType() == vtkTypeTraits<Scalar>::VTK_TYPE_ID)
        {
        return static_cast<vtkTypedDataArray<Scalar>*>(source);
        }
    default:
      return NULL;
    }
}

#endif //__vtkTypedDataArray_txx
