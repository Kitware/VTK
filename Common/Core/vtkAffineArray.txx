// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkAffineArray_txx
#define vtkAffineArray_txx

#ifdef VTK_AFFINE_ARRAY_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkAffineArray.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkAffineArray<ValueTypeT>* vtkAffineArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkAffineArray<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkAffineArray<ValueTypeT>::ConstructBackend(ValueType slope, ValueType intercept)
{
  this->Superclass::ConstructBackend(slope, intercept);
}

VTK_ABI_NAMESPACE_END
#endif // header guard
