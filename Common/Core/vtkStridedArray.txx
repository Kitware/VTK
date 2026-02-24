// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkStridedArray_txx
#define vtkStridedArray_txx

#ifdef VTK_STRIDED_ARRAY_INSTANTIATING
#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#undef VTK_GDA_VALUERANGE_INSTANTIATING
#endif

#include "vtkStridedArray.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkStridedArray<ValueTypeT>* vtkStridedArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkStridedArray<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStridedArray<ValueTypeT>::ConstructBackend(
  const ValueType* buffer, vtkIdType stride, int components, vtkIdType offset)
{
  this->Superclass::ConstructBackend(buffer, stride, components, offset);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStridedArray<ValueTypeT>::ConstructBackend(
  const ValueType* buffer, vtkIdType stride, int components)
{
  this->Superclass::ConstructBackend(buffer, stride, components);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStridedArray<ValueTypeT>::ConstructBackend(const ValueType* buffer, vtkIdType stride)
{
  this->Superclass::ConstructBackend(buffer, stride);
}

VTK_ABI_NAMESPACE_END
#endif // header guard
