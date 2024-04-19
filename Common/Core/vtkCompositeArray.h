// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkCompositeArray_h
#define vtkCompositeArray_h

#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonCoreModule.h"         // for export macro
#include "vtkCompositeImplicitBackend.h" // for the array backend
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
// The instantiation is separated in two functions because the .txx includes vtkArrayDispatch.h
// which when Dispatching is enabled, it instantiates a class with a value type, before exporting it
#define VTK_INSTANTIATE_COMPOSITE_ARRAY(ValueType)                                                 \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>;    \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>, double)                              \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#define VTK_INSTANTIATE_COMPOSITE_ARRAY_FUNCTIONS(ValueType)                                       \
  namespace vtk                                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template VTKCOMMONCORE_EXPORT                                                                    \
    vtkSmartPointer<vtkImplicitArray<vtkCompositeImplicitBackend<ValueType>>>                      \
    ConcatenateDataArrays(const std::vector<vtkDataArray*>& arrays);                               \
  VTK_ABI_NAMESPACE_END                                                                            \
  }

#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_COMPOSITE_ARRAY_TEMPLATE_EXTERN
#define VTK_COMPOSITE_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkCompositeArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternSecondOrderTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkCompositeImplicitBackend);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
VTK_ABI_NAMESPACE_END
#endif // VTK_COMPOSITE_ARRAY_TEMPLATE_EXTERN
// The following clause is only for MSVC 2008 and 2010
#elif defined(_MSC_VER) && !defined(VTK_BUILD_SHARED_LIBS)
#pragma warning(push)
// C4091: 'extern ' : ignored on left of 'int' when no variable is declared
#pragma warning(disable : 4091)

// Compiler-specific extension warning.
#pragma warning(disable : 4231)

// We need to disable warning 4910 and do an extern dllexport
// anyway.  When deriving new arrays from an
// instantiation of this template the compiler does an explicit
// instantiation of the base class.  From outside the vtkCommon
// library we block this using an extern dllimport instantiation.
// For classes inside vtkCommon we should be able to just do an
// extern instantiation, but VS 2008 complains about missing
// definitions.  We cannot do an extern dllimport inside vtkCommon
// since the symbols are local to the dll.  An extern dllexport
// seems to be the only way to convince VS 2008 to do the right
// thing, so we just disable the warning.
#pragma warning(disable : 4910) // extern and dllexport incompatible

// Use an "extern explicit instantiation" to give the class a DLL
// interface.  This is a compiler-specific extension.
VTK_ABI_NAMESPACE_BEGIN
vtkInstantiateSecondOrderTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkCompositeImplicitBackend);

#pragma warning(pop)

VTK_ABI_NAMESPACE_END
#endif
