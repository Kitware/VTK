// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkTypedDataArray_txx
#define vtkTypedDataArray_txx

#include "vtkTypedDataArray.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <typename Scalar>
vtkTypedDataArray<Scalar>::vtkTypedDataArray() = default;

//------------------------------------------------------------------------------
template <typename Scalar>
vtkTypedDataArray<Scalar>::~vtkTypedDataArray() = default;

//------------------------------------------------------------------------------
template <typename Scalar>
bool vtkTypedDataArray<Scalar>::AllocateTuples(vtkIdType)
{
  vtkErrorMacro(<< "This method is not preferred for vtkTypedDataArray "
                   "implementations. Either add an appropriate implementation, or "
                   "use Allocate instead.");
  return false;
}

//------------------------------------------------------------------------------
template <typename Scalar>
bool vtkTypedDataArray<Scalar>::ReallocateTuples(vtkIdType)
{
  vtkErrorMacro(<< "This method is not preferred for vtkTypedDataArray "
                   "implementations. Either add an appropriate implementation, or "
                   "use Resize instead.");
  return false;
}

//------------------------------------------------------------------------------
template <typename Scalar>
inline int vtkTypedDataArray<Scalar>::GetDataType() const
{
  return vtkTypeTraits<Scalar>::VTK_TYPE_ID;
}

//------------------------------------------------------------------------------
template <typename Scalar>
inline int vtkTypedDataArray<Scalar>::GetDataTypeSize() const
{
  return static_cast<int>(sizeof(Scalar));
}

//------------------------------------------------------------------------------
template <typename Scalar>
inline typename vtkTypedDataArray<Scalar>::ValueType vtkTypedDataArray<Scalar>::GetTypedComponent(
  vtkIdType tupleIdx, int comp) const
{
  return this->GetValue(tupleIdx * this->NumberOfComponents + comp);
}

//------------------------------------------------------------------------------
template <typename Scalar>
inline void vtkTypedDataArray<Scalar>::SetTypedComponent(vtkIdType tupleIdx, int comp, ValueType v)
{
  this->SetValue(tupleIdx * this->NumberOfComponents + comp, v);
}

//------------------------------------------------------------------------------
template <typename Scalar>
inline vtkTypedDataArray<Scalar>* vtkTypedDataArray<Scalar>::FastDownCast(vtkAbstractArray* source)
{
  if (source)
  {
    switch (source->GetArrayType())
    {
      case vtkAbstractArray::TypedDataArray:
      case vtkAbstractArray::MappedDataArray:
        if (vtkDataTypesCompare(source->GetDataType(), vtkTypeTraits<Scalar>::VTK_TYPE_ID))
        {
          return static_cast<vtkTypedDataArray<Scalar>*>(source);
        }
        break;
    }
  }
  return nullptr;
}

VTK_ABI_NAMESPACE_END
#endif // vtkTypedDataArray_txx
