/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIndexedImplicitBackend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkIndexedImplicitBackend_h
#define vtkIndexedImplicitBackend_h

/**
 * \class vtkIndexedImplicitBackend
 *
 * A backend for the `vtkImplicitArray` framework allowing one to use a subset of a given data
 * array, by providing a `vtkIdList` or `vtkDataArray` of indexes as indirection, as another
 * `vtkDataArray` without any excess memory consumption.
 *
 * This structure can be classified as a closure and can be called using syntax similar to a
 * function call.
 *
 * An example of potential usage in a `vtkImplicitArray`:
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
 * vtkNew<vtkImplicitArray<vtkIndexedImplicitBackend<int>>> indexedArr; // More compact with
 *                                                                      // `vtkIndexedArray<int>`
 *                                                                      // if available
 * indexedArr->SetBackend(std::make_shared<vtkIndexedImplicitBackend<int>>(handles, baseArray));
 * indexedArr->SetNumberOfComponents(1);
 * indexedArr->SetNumberOfTuples(100);
 * CHECK(indexedArr->GetValue(57) == 42);
 * ```
 *
 * @sa
 * vtkImplicitArray, vtkIndexedArray
 */

#include "vtkCommonImplicitArraysModule.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkIdList;
template <typename ValueType>
class vtkIndexedImplicitBackend final
{
public:
  ///@{
  /**
   * Constructor
   * @param indexes list of indexes to use for indirection of the array
   * @param array base array of interest
   */
  vtkIndexedImplicitBackend(vtkIdList* indexes, vtkDataArray* array);
  vtkIndexedImplicitBackend(vtkDataArray* indexes, vtkDataArray* array);
  ///@}
  ~vtkIndexedImplicitBackend();

  /**
   * Indexing operation for the indexed array respecting the backend expectations of
   * `vtkImplicitArray`
   */
  ValueType operator()(int idx) const;

private:
  struct Internals;
  std::unique_ptr<Internals> Internal;
};
VTK_ABI_NAMESPACE_END

#endif // vtkIndexedImplicitBackend_h

#ifdef VTK_INDEXED_BACKEND_INSTANTIATING
#define VTK_INSTANTIATE_INDEXED_BACKEND(ValueType)                                                 \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT vtkIndexedImplicitBackend<ValueType>;              \
  VTK_ABI_NAMESPACE_END
#endif
