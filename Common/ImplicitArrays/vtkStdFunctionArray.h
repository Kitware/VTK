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
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkStdFunctionArray_h
#define vtkStdFunctionArray_h

#ifdef VTK_STD_FUNCTION_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonImplicitArraysModule.h" // for export macro
#include "vtkImplicitArray.h"

#ifdef VTK_STD_FUNCTION_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

#include <functional>

/**
 * \var vtkStdFunctionArray
 * \brief A utility alias for wrapping std::function in implicit arrays
 *
 * The main goal behind this alias is to be able to offer some semi-flexible instantiations of
 * implicit arrays that can work with the vtkArrayDispatch mechanisms.
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time. As such, they need to be compilable without knowing the exact
 * function/mapping to include in the backend. This is why std::function is used as the backend
 * here.
 *
 * @sa
 * vtkImplicitArray
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
using vtkStdFunctionArray = vtkImplicitArray<std::function<T(int)>>;
VTK_ABI_NAMESPACE_END

#endif // vtkStdFunctionArray_h

#ifdef VTK_STD_FUNCTION_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_STD_FUNCTION_ARRAY(ValueType)                                              \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT vtkImplicitArray<std::function<ValueType(int)>>;   \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkImplicitArray<std::function<ValueType(int)>>, double)    \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#endif
