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
 * @sa
 * vtkImplicitArray vtkCompositeImplicitBackend
 */

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
template <typename T>
using vtkCompositeArray = vtkImplicitArray<vtkCompositeImplicitBackend<T>>;
VTK_ABI_NAMESPACE_END

namespace vtkCompositeArrayUtilities
{
VTK_ABI_NAMESPACE_BEGIN
template <typename T>
vtkSmartPointer<vtkCompositeArray<T>> Concatenate(const std::vector<vtkDataArray*>& arrays);
VTK_ABI_NAMESPACE_END
}

#endif // vtkCompositeArray_h

#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_COMPOSITE_ARRAY(ValueType)                                                 \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONIMPLICITARRAYS_EXPORT                                                    \
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>;                                      \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkCompositeArrayUtilities                                                             \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template VTKCOMMONIMPLICITARRAYS_EXPORT                                                          \
    vtkSmartPointer<vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>>                      \
    Concatenate(const std::vector<vtkDataArray*>& arrays);                                         \
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
