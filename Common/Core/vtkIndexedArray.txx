// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkIndexedArray_txx
#define vtkIndexedArray_txx

#ifdef VTK_INDEXED_ARRAY_INSTANTIATING
#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#undef VTK_GDA_VALUERANGE_INSTANTIATING
#endif

#include "vtkIndexedArray.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkIndexedArray<ValueTypeT>* vtkIndexedArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkIndexedArray<ValueType>);
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
void vtkIndexedArray<ValueTypeT>::ConstructBackend(vtkIdList* indexes, vtkDataArray* array)
{
  this->Superclass::ConstructBackend(indexes, array);
}

//------------------------------------------------------------------------------
template <class ValueTypeT>
void vtkIndexedArray<ValueTypeT>::ConstructBackend(vtkDataArray* indexes, vtkDataArray* array)
{
  this->Superclass::ConstructBackend(indexes, array);
}

VTK_ABI_NAMESPACE_END
#endif // header guard
