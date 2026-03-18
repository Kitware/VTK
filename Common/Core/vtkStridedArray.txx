// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkStridedArray_txx
#define vtkStridedArray_txx

#ifdef VTK_STRIDED_ARRAY_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkStridedArray.h"

#include "vtkAbstractBuffer.h"

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

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStridedArray<ValueTypeT>::ConstructBackend(
  vtkAbstractBuffer* buffer, vtkIdType stride, int components, vtkIdType offset)
{
  this->BufferSource = static_cast<vtkBuffer<ValueTypeT>*>(buffer);
  this->SetNumberOfComponents(components);
  vtkIdType bufferSize = buffer->GetNumberOfElements();
  vtkIdType ntuples = (bufferSize - offset - components) / stride + 1;
  this->SetNumberOfTuples(ntuples);
  const ValueType* rawPtr = static_cast<const ValueType*>(buffer->GetVoidBuffer());
  this->ConstructBackend(rawPtr, stride, components, offset);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStridedArray<ValueTypeT>::ConstructBackend(
  vtkAbstractBuffer* buffer, vtkIdType stride, int components)
{
  this->ConstructBackend(buffer, stride, components, 0);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStridedArray<ValueTypeT>::ConstructBackend(vtkAbstractBuffer* buffer, vtkIdType stride)
{
  this->ConstructBackend(buffer, stride, 1, 0);
}

VTK_ABI_NAMESPACE_END
#endif // header guard
