/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStdFunctionArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkStdFunctionArray_h
#define vtkStdFunctionArray_h

#ifdef VTK_STD_FUNCTION_ARRAY_INSTANTIATING
#define VTK_GDA_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#include "vtkGenericDataArray.h"
#undef VTK_GDA_VALUERANGE_INSTANTIATING
#endif

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkImplicitArray.h"

#include <functional>

template <typename T>
using vtkStdFunctionArray = vtkImplicitArray<std::function<T(int)>>;

#endif // vtkStdFunctionArray_h

#ifdef VTK_STD_FUNCTION_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_STD_FUNCTION_ARRAY(ValueType)                                              \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkStdFunctionArray<ValueType>, double)                     \
  }

#endif // VTK_STD_FUNCTION_ARRAY_INSTANTIATING
