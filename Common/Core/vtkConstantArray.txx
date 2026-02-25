// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkConstantArray_txx
#define vtkConstantArray_txx

#ifdef VTK_CONSTANT_ARRAY_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkConstantArray.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkConstantArray<ValueTypeT>* vtkConstantArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkConstantArray<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkConstantArray<ValueTypeT>::ConstructBackend(ValueType value)
{
  this->Superclass::ConstructBackend(value);
}

VTK_ABI_NAMESPACE_END
#endif // header guard
