// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkImplicitArray_txx
#define vtkImplicitArray_txx

#include "vtkAOSDataArrayTemplate.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
vtkImplicitArray<BackendT, ArrayType>* vtkImplicitArray<BackendT, ArrayType>::New()
{
  using vtkImplicitArrayType = vtkImplicitArray<BackendT, ArrayType>;
  VTK_STANDARD_NEW_BODY(vtkImplicitArrayType);
}

//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
vtkImplicitArray<BackendT, ArrayType>::vtkImplicitArray()
{
  this->Initialize();
}

//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
vtkImplicitArray<BackendT, ArrayType>::~vtkImplicitArray() = default;

//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
void vtkImplicitArray<BackendT, ArrayType>::SetValue(
  vtkIdType vtkNotUsed(idx), ValueType vtkNotUsed(value))
{
}

//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
void vtkImplicitArray<BackendT, ArrayType>::SetTypedTuple(
  vtkIdType vtkNotUsed(idx), const ValueType* vtkNotUsed(tuple))
{
}

//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
void vtkImplicitArray<BackendT, ArrayType>::SetTypedComponent(
  vtkIdType vtkNotUsed(idx), int vtkNotUsed(comp), ValueType vtkNotUsed(value))
{
}

//-----------------------------------------------------------------------------
template <class BackendT, int ArrayType>
vtkImplicitArray<BackendT, ArrayType>* vtkImplicitArray<BackendT, ArrayType>::FastDownCast(
  vtkAbstractArray* source)
{
  if (source)
  {
    if constexpr (vtkImplicitArray::ArrayTypeTag::value != vtkArrayTypes::VTK_IMPLICIT_ARRAY)
    {
      return Superclass::FastDownCast(source);
    }
    else
    {
      switch (source->GetArrayType())
      {
        case vtkArrayTypes::VTK_IMPLICIT_ARRAY:
          if (vtkDataTypesCompare(source->GetDataType(), vtkImplicitArray::DataTypeTag::value))
          {
            // The problem here is that we do not know what type of backend to use and any pointer
            // to an implicit array will down cast to any other pointer to an implicit array.
            // Barring something better to do here, we use the SafeDownCast mechanism to ensure
            // safety at the cost of performance.
            return vtkImplicitArray<BackendT, ArrayType>::SafeDownCast(source);
          }
          break;
      }
    }
  }
  return nullptr;
}

VTK_ABI_NAMESPACE_END

#endif // vtkImplicitArray_txx
