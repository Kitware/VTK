/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeImplicitBackend.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkCompositeImplicitBackend_h
#define vtkCompositeImplicitBackend_h

/**
 * \class vtkCompositeImplicitBackend
 * \brief A utility structure serving as a backend for composite arrays: an array composed of
 * multiple arrays concatenated together
 *
 * This structure can be classified as a closure and can be called using syntax similar to a
 * function call.
 *
 * This class uses indirect addressing of cached arrays to provide an access compute complexity of
 * O(log_2(number_of_arrays)) through its `()` operator.
 *
 * At construction it takes an array arrays in order to represent their concatenation.
 *
 * An example of potential usage in a vtkImplicitArray
 * ```
 * vtkNew<vtkIntArray> leftArr;
 * leftArr->SetNumberOfComponents(1);
 * leftArr->SetNumberOfTuples(1);
 * leftArr->SetValue(0, 0);
 * vtkNew<vtkIntArray> rightArr;
 * rightArr->SetNumberOfComponents(1);
 * rightArr->SetNumberOfTuples(1);
 * rightArr->SetValue(0, 1);
 * vtkNew<vtkImplicitArray<vtkCompositeImplicitBackend<int>>> compositeArr; // easier with
 * `vtkNew<vtkCompositeArray<int>> compositeArr;` if applicable
 * std::vector<vtkDataArray*> arrays({leftArr, rightArr});
 * compositeArr->SetBackend(std::make_shared<vtkCompositeImplicitBackend<int>>(arrays));
 * CHECK(compositArr->GetValue(1) == 1);
 * ```
 *
 * > WARNING:
 * > Arrays input to the backend are flattened upon use and are no longer sensitive to component
 * > information.
 */
#include "vtkCommonImplicitArraysModule.h"

#include <memory>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
template <typename ValueType>
class vtkCompositeImplicitBackend final
{
public:
  /**
   * Constructor for the backend
   * @param arrays std::vector of arrays to composite together
   * leftArr->GetNumberOfTuples()
   */
  vtkCompositeImplicitBackend(const std::vector<vtkDataArray*>& arrays);
  ~vtkCompositeImplicitBackend();

  /**
   * Indexing operator for the composite of the two arrays respecting the `vtkImplicitArray`
   * expectations.
   *
   * Conceptually, the composite array uses a binary search algorithm through the use of
   * `std::upper_bounds` to offer a compute complexity of O(log_2(n_arrays))
   */
  ValueType operator()(int idx) const;

protected:
  struct Internals;
  std::unique_ptr<Internals> Internal;
};
VTK_ABI_NAMESPACE_END

#endif // vtkCompositeImplicitBackend_h

#ifdef VTK_COMPOSITE_BACKEND_INSTANTIATING
#define VTK_INSTANTIATE_COMPOSITE_BACKEND(ValueType)                                               \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT vtkCompositeImplicitBackend<ValueType>;            \
  VTK_ABI_NAMESPACE_END
#endif
