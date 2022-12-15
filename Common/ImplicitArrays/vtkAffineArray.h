/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAffineArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
