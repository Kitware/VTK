// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkIndexedArray_h
#define vtkIndexedArray_h

#ifdef VTK_INDEXED_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonCoreModule.h" // for export macro
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
  template class VTKCOMMONCORE_EXPORT vtkImplicitArray<vtkIndexedImplicitBackend<ValueType>>;      \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkIndexedImplicitBackend<ValueType>>, double)                                \
  VTK_ABI_NAMESPACE_END                                                                            \
  }

#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_INDEXED_ARRAY_TEMPLATE_EXTERN
#define VTK_INDEXED_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkIndexedArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternSecondOrderTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkIndexedImplicitBackend);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
VTK_ABI_NAMESPACE_END
#endif // VTK_INDEXED_ARRAY_TEMPLATE_EXTERN
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
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkIndexedImplicitBackend);

#pragma warning(pop)

VTK_ABI_NAMESPACE_END
#endif
