/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkCompositeArray_h
#define vtkCompositeArray_h

#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonImplicitArraysModule.h" // for export macro
#include "vtkCompositeImplicitBackend.h"   // for the array backend
#include "vtkImplicitArray.h"

#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

#include <vector>

/**
 * \var vtkCompositeArray
 * \brief A utility alias for concatenating arrays into an implicit array
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time.
 *
 * An example of potential usage
 * ```
 * vtkNew<vtkIntArray> leftArr;
 * leftArr->SetNumberOfComponents(1);
 * leftArr->SetNumberOfTuples(1);
 * leftArr->SetValue(0, 0);
 * vtkNew<vtkIntArray> rightArr;
 * rightArr->SetNumberOfComponents(1);
 * rightArr->SetNumberOfTuples(1);
 * rightArr->SetValue(0, 1);
 * std::vector<vtkDataArray*> arrays({leftArr, rightArr});
 * vtkNew<vtkCompositeArray<int>> compositeArr;
 * compositeArr->SetBackend(std::make_shared<vtkCompositeImplicitBackend<int>>(arrays));
 * compositeArr->SetNumberOfComponents(1);
 * compositeArr->SetNumberOfTuples(2);
 * CHECK(compositArr->GetValue(1) == 1);
 * ```
 * @sa
 * vtkImplicitArray vtkCompositeImplicitBackend
 */

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
template <typename T>
using vtkCompositeArray = vtkImplicitArray<vtkCompositeImplicitBackend<T>>;
VTK_ABI_NAMESPACE_END

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
/**
 * \fn ConcatenateDataArrays
 * A method that can take a `std::vector` of `vtkDataArray`s and concatenate them together into a
 * single `vtkCompositeArray`. Input arrays should all have the same number of components and the
 * resulting composite array has as many tuples as the sum of all the inputs.
 *
 * The method is templated based on the value type of composite array the caller wishes as a result.
 */
vtkSmartPointer<vtkCompositeArray<T>> ConcatenateDataArrays(
  const std::vector<vtkDataArray*>& arrays);
VTK_ABI_NAMESPACE_END
}

#endif // vtkCompositeArray_h

#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_COMPOSITE_ARRAY(ValueType)                                                 \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT                                                    \
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>;                                      \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtk                                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template VTKCOMMONIMPLICITARRAYS_EXPORT                                                          \
    vtkSmartPointer<vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>>                      \
    ConcatenateDataArrays(const std::vector<vtkDataArray*>& arrays);                               \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>, double)                              \
  VTK_ABI_NAMESPACE_END                                                                            \
  }

#endif
