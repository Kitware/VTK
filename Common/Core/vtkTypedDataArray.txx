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

#ifndef vtkTypedDataArray_txx
#define vtkTypedDataArray_txx

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
template <typename Scalar>
bool vtkTypedDataArray<Scalar>::AllocateTuples(vtkIdType)
{
  vtkErrorMacro(<<"This method is not preferred for vtkTypedDataArray "
                "implementations. Either add an appropriate implementation, or "
                "use Allocate instead.");
  return false;
}

//------------------------------------------------------------------------------
template <typename Scalar>
bool vtkTypedDataArray<Scalar>::ReallocateTuples(vtkIdType)
{
  vtkErrorMacro(<<"This method is not preferred for vtkTypedDataArray "
                "implementations. Either add an appropriate implementation, or "
                "use Resize instead.");
  return false;
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
template <typename Scalar> inline
typename vtkTypedDataArray<Scalar>::ValueType
vtkTypedDataArray<Scalar>::GetTypedComponent(vtkIdType tupleIdx, int comp) const
{
  return this->GetValue(tupleIdx * this->NumberOfComponents + comp);
}

//------------------------------------------------------------------------------
template <typename Scalar> inline
void vtkTypedDataArray<Scalar>::SetTypedComponent(vtkIdType tupleIdx, int comp,
                                                  ValueType v)
{
  this->SetValue(tupleIdx * this->NumberOfComponents + comp, v);
}

//------------------------------------------------------------------------------
template <typename Scalar> inline vtkTypedDataArray<Scalar> *
vtkTypedDataArray<Scalar>::FastDownCast(vtkAbstractArray *source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkAbstractArray::TypedDataArray:
      case vtkAbstractArray::MappedDataArray:
        if (vtkDataTypesCompare(source->GetDataType(),
                                vtkTypeTraits<Scalar>::VTK_TYPE_ID))
        {
          return static_cast<vtkTypedDataArray<Scalar>*>(source);
        }
        break;
    }
  }
  return nullptr;
}

#endif //vtkTypedDataArray_txx
