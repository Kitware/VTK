// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkStridedImplicitBackend.h"

#include "vtkArrayDispatch.h"
#include "vtkDataArrayRange.h"
#include "vtkIdList.h"
#include "vtkImplicitArray.h"
#include "vtkTypeList.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
//-----------------------------------------------------------------------
template <typename ValueType>
vtkStridedImplicitBackend<ValueType>::vtkStridedImplicitBackend(
  const ValueType* buffer, vtkIdType stride, int components, vtkIdType offset)
  : Buffer(buffer)
  , Stride(stride)
  , Offset(offset)
  , NumberOfComponents(components)
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkStridedImplicitBackend<ValueType>::vtkStridedImplicitBackend(
  const ValueType* buffer, vtkIdType stride, int components)
  : Buffer(buffer)
  , Stride(stride)
  , NumberOfComponents(components)
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkStridedImplicitBackend<ValueType>::vtkStridedImplicitBackend(
  const ValueType* buffer, vtkIdType stride)
  : Buffer(buffer)
  , Stride(stride)
{
}

//-----------------------------------------------------------------------
template <typename ValueType>
vtkStridedImplicitBackend<ValueType>::~vtkStridedImplicitBackend() = default;

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkStridedImplicitBackend<ValueType>::operator()(vtkIdType idx) const
{
  vtkIdType tuple = idx / this->NumberOfComponents;
  vtkIdType comp = idx % this->NumberOfComponents;
  return this->Buffer[tuple * this->Stride + this->Offset + comp];
}

//-----------------------------------------------------------------------
template <typename ValueType>
void vtkStridedImplicitBackend<ValueType>::mapTuple(vtkIdType tupleIdx, ValueType* tuple) const
{
  vtkIdType tupleStart = tupleIdx * this->Stride + this->Offset;
  for (int comp = 0; comp < this->NumberOfComponents; comp++)
  {
    tuple[comp] = this->Buffer[tupleStart + comp];
  }
}

//-----------------------------------------------------------------------
template <typename ValueType>
ValueType vtkStridedImplicitBackend<ValueType>::mapComponent(vtkIdType tupleIdx, int compIdx) const
{
  return this->Buffer[tupleIdx * this->Stride + this->Offset + compIdx];
}
VTK_ABI_NAMESPACE_END
