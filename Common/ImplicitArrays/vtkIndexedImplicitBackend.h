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
#ifndef vtkIndexedImplicitBackend_h
#define vtkIndexedImplicitBackend_h

#include "vtkCommonImplicitArraysModule.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkIdList;
template <typename ValueType>
class vtkIndexedImplicitBackend
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
