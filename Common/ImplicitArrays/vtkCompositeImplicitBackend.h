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
 * The class implements a binary tree like structure for facilitating fast access.
 *
 * At construction it takes two arrays in order to represent their concatenation.
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
 * compositeArr->SetBackend(std::make_shared<vtkCompositeImplicitBackend<int>>(leftArr, rightArr));
 * CHECK(compositArr->GetValue(1) == 1);
 * ```
 */
#include "vtkCommonImplicitArraysModule.h"
#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
template <typename ValueType>
class vtkCompositeImplicitBackend
{
public:
  /**
   * Constructor for the backend
   * @param leftArr the array starting the composite at index 0
   * @param rightArr the array following the leftArr and starting at index
   * leftArr->GetNumberOfTuples()
   */
  vtkCompositeImplicitBackend(vtkDataArray* leftArr, vtkDataArray* rightArr);
  ~vtkCompositeImplicitBackend();

  /**
   * Indexing operator for the composite of the two arrays respecting the `vtkImplicitArray`
   * expectations
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
