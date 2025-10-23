// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStridedArray_h
#define vtkStridedArray_h

#ifdef VTK_STRIDED_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkImplicitArray.h"
#include "vtkStridedImplicitBackend.h" // for the array backend

#ifdef VTK_STRIDED_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

/**
 * \var vtkStridedArray
 * \brief
 *
 * An implicit array to create a strided view on a buffer.
 *
 * Starting with a multi-dimensional buffer of data, a vtkStridedArray can provide
 * the vtkDataArray interface on only one dimension.
 *
 * An example of potential usage:
 * ```
 * std::vector<float> localBuffer
 * {
 *       1000, 2000, 3000,
 *       1001, 2001, 3001,
 *       1002, 2002, 3002,
 *       1003, 2003, 3003,
 *       1004, 2004, 3004,
 *       1005, 2005, 3005,
 *       1006, 2006, 3006,
 *       1007, 2007, 3007,
 *       1008, 2008, 3008,
 *       1009, 2009, 3009
 * }
 *
 * vtkNew<vtkStridedArray<float>> stridedArray;
 * stridedArray->SetNumberOfComponents(2);
 * stridedArray->SetNumberOfTuples(10);
 * const int stride = 3;
 * const int offset = 1;
 * const int comp = 2;
 * stridedArray->ConstructBackend(localBuffer.data(), stride, comp, offset);
 *
 * CHECK(stridedArr->GetComponent(2, 1) == 3002); // true
 * // more generically:
 * stridedArr->GetComponent(tupleIdx, compIdx) = buffer[offset + compIdx + tupleIdx * stride];
 * ```
 *
 * You can see the stride as the number of components of the buffer.
 *
 * @warning The buffer is not owned by the `vtkStridedArray`: do not try to use
 * the `vtkStridedArray` after the buffer memory is released.
 *
 * @note The different components of the array should be contiguous.
 * vtkStridedArray supports a global stride but not a stride between component.
 * In the previous example, one cannot create a vtkStridedArray using the `100X` series
 * as first component and the `300X` series as a second component.
 *
 * @sa
 * vtkImplicitArray vtkStridedImplicitBackend
 */

VTK_ABI_NAMESPACE_BEGIN
template <typename T>
using vtkStridedArray =
  vtkImplicitArray<vtkStridedImplicitBackend<T>, vtkArrayTypes::VTK_STRIDED_ARRAY>;
VTK_ABI_NAMESPACE_END

#endif // vtkStridedArray_h

#ifdef VTK_STRIDED_ARRAY_INSTANTIATING

#define VTK_INSTANTIATE_STRIDED_ARRAY(ValueType)                                                   \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT                                                              \
    vtkImplicitArray<vtkStridedImplicitBackend<ValueType>, vtkArrayTypes::VTK_STRIDED_ARRAY>;      \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    VTK_WRAP_TEMPLATE(                                                                             \
      vtkImplicitArray<vtkStridedImplicitBackend<ValueType>, vtkArrayTypes::VTK_STRIDED_ARRAY>),   \
    double)                                                                                        \
  VTK_ABI_NAMESPACE_END                                                                            \
  }

#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_STRIDED_ARRAY_TEMPLATE_EXTERN
#define VTK_STRIDED_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkStridedArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternSecondOrderWithParameterTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkStridedImplicitBackend,
  vtkArrayTypes::VTK_STRIDED_ARRAY);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
VTK_ABI_NAMESPACE_END
#endif // VTK_STRIDED_ARRAY_TEMPLATE_EXTERN
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
vtkInstantiateSecondOrderWithParameterTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkStridedImplicitBackend,
  vtkArrayTypes::VTK_STRIDED_ARRAY);

#pragma warning(pop)

VTK_ABI_NAMESPACE_END
#endif
