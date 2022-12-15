/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIndexedArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkIndexedArray_h
#define vtkIndexedArray_h

#ifdef VTK_INDEXED_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonImplicitArraysModule.h" // for export macro
#include "vtkImplicitArray.h"
#include "vtkIndexedImplicitBackend.h" // for the array backend

#ifdef VTK_INDEXED_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

#include <vector>

/**
 * \var vtkIndexedArray
 * \brief A utility alias for creating a wrapper array around an existing array and reindexing its
 * components
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time.
 *
 * An example of potential usage:
 * ```
 * vtkNew<vtkIntArray> baseArray;
 * baseArray->SetNumberOfComponents(1);
 * baseArray->SetNumberOfTuples(100);
 * auto range = vtk::DataArrayValueRange<1>(baseArray);
 * std::iota(range.begin(), range.end(), 0);
 *
 * vtkNew<vtkIdList> handles;
 * handles->SetNumberOfIds(100);
 * for (vtkIdType idx = 0; idx < 100; idx++)
 * {
 *   handles->SetId(idx, 99-idx);
 * }
 *
 * vtkNew<vtkIndexed<int>> indexedArr;
 * indexedArr->SetBackend(std::make_shared<vtkIndexedImplicitBackend<int>>(handles, baseArray));
 * indexedArr->SetNumberOfComponents(1);
 * indexedArr->SetNumberOfTuples(100);
 * CHECK(indexedArr->GetValue(57) == 42); // always true
 * ```
 *
 * @sa
 * vtkImplicitArray vtkIndexedImplicitBackend
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
using vtkIndexedArray = vtkImplicitArray<vtkIndexedImplicitBackend<T>>;
VTK_ABI_NAMESPACE_END

#endif // vtkIndexedArray_h

#ifdef VTK_INDEXED_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_INDEXED_ARRAY(ValueType)                                                   \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT                                                    \
    vtkImplicitArray<vtkIndexedImplicitBackend<ValueType>>;                                        \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkIndexedImplicitBackend<ValueType>>, double)                                \
  VTK_ABI_NAMESPACE_END                                                                            \
  }

#endif
