// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkStdFunctionArray_txx
#define vtkStdFunctionArray_txx

#ifdef VTK_STD_FUNCTION_ARRAY_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkStdFunctionArray.h"

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
vtkStdFunctionArray<ValueTypeT>* vtkStdFunctionArray<ValueTypeT>::New()
{
  VTK_STANDARD_NEW_BODY(vtkStdFunctionArray<ValueType>);
}

//-----------------------------------------------------------------------------
template <class ValueTypeT>
void vtkStdFunctionArray<ValueTypeT>::ConstructBackend(std::function<ValueType(int)> func)
{
  this->Superclass::ConstructBackend(func);
}

VTK_ABI_NAMESPACE_END
#endif // header guard
