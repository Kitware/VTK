// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkAffineArray_h
#define vtkAffineArray_h

#ifdef VTK_AFFINE_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkAffineImplicitBackend.h"      // for the array backend
#include "vtkCommonImplicitArraysModule.h" // for export macro
#include "vtkImplicitArray.h"

#ifdef VTK_AFFINE_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

/**
 * \var vtkAffineArray
 * \brief A utility alias for wrapping affine functions in implicit arrays
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time.
 *
 * @sa
 * vtkImplicitArray vtkAffineImplicitBackend
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
using vtkAffineArray = vtkImplicitArray<vtkAffineImplicitBackend<T>>;
VTK_ABI_NAMESPACE_END

#endif // vtkAffineArray_h

#ifdef VTK_AFFINE_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_AFFINE_ARRAY(ValueType)                                                    \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT                                                    \
    vtkImplicitArray<vtkAffineImplicitBackend<ValueType>>;                                         \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkAffineImplicitBackend<ValueType>>, double)                                 \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#endif
