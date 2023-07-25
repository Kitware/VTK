// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkConstantArray_h
#define vtkConstantArray_h

#ifdef VTK_CONSTANT_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonImplicitArraysModule.h" // for export macro
#include "vtkConstantImplicitBackend.h"    // for the array backend
#include "vtkImplicitArray.h"

#ifdef VTK_CONSTANT_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

/**
 * \var vtkConstantArray
 * \brief A utility alias for wrapping constant functions in implicit arrays
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time.
 *
 * @sa
 * vtkImplicitArray vtkConstantImplicitBackend
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
using vtkConstantArray = vtkImplicitArray<vtkConstantImplicitBackend<T>>;
VTK_ABI_NAMESPACE_END

#endif // vtkConstantArray_h

#ifdef VTK_CONSTANT_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_CONSTANT_ARRAY(ValueType)                                                  \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT                                                    \
    vtkImplicitArray<vtkConstantImplicitBackend<ValueType>>;                                       \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkConstantImplicitBackend<ValueType>>, double)                               \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#endif
