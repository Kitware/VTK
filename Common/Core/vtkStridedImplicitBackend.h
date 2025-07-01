// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStridedImplicitBackend_h
#define vtkStridedImplicitBackend_h

/**
 * \class vtkStridedImplicitBackend
 *
 * A backend for the `vtkImplicitArray` framework to give a strided view on a buffer.
 *
 * @warning The buffer is not owned by the backend: do not try to use
 * the `vtkImplicitArray` after the buffer memory is released.
 *
 * See `vtkStridedArray` for an example of usage.
 *
 * @sa
 * vtkImplicitArray, vtkStridedArray
 */

#include "vtkCommonCoreModule.h"

#include "vtkType.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
template <typename ValueType>
class VTKCOMMONCORE_EXPORT vtkStridedImplicitBackend final
{
public:
  ///@{
  /**
   * Constructors.
   * - stride is the number of values in a buffer tuple.
   * - components is the number of components of the resulting array, usually less than stride.
   * - offset is the the number of buffer values to skip to get the first array value.
   * In other words:
   * - the constructed array starts at buffer[offset].
   * - the component "i" of first tuple is at buffer[offset + i]
   * - the tuple "n" starts at buffer[stride * n + offset]
   * - so the component "i" of the tuple "n" is at buffer[stride * n + offset + i]
   */
  vtkStridedImplicitBackend(
    const ValueType* buffer, vtkIdType stride, int components, vtkIdType offset);
  vtkStridedImplicitBackend(const ValueType* buffer, vtkIdType stride, int components);
  vtkStridedImplicitBackend(const ValueType* buffer, vtkIdType stride);
  ///@}
  ~vtkStridedImplicitBackend();

  /**
   * Return the value at the given flat idx.
   * This is equivalent to mapComponent(tupleIdx, compIdx)
   * where `idx = tupleIdx * NumberOfComponents + compIdx`
   */
  ValueType operator()(vtkIdType idx) const;

  /**
   * Fill tuple with the content of the tupleIdx of the array.
   * In the buffer, this is buffer[this->Stride * tupleIdx + this->Offset].
   */
  void mapTuple(vtkIdType tupleIdx, ValueType* tuple) const;

  /**
   * Return the specified component value.
   * It is at buffer[this->Stride * tupleIdx + compIdx + this->Offset]
   */
  ValueType mapComponent(vtkIdType tupleIdx, int compIdx) const;

private:
  const ValueType* Buffer = nullptr;
  vtkIdType Stride = 1;
  vtkIdType Offset = 0;
  int NumberOfComponents = 1;
};
VTK_ABI_NAMESPACE_END

#endif // vtkStridedImplicitBackend_h

#if defined(VTK_STRIDED_BACKEND_INSTANTIATING)

#define VTK_INSTANTIATE_STRIDED_BACKEND(ValueType)                                                 \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkStridedImplicitBackend<ValueType>;                        \
  VTK_ABI_NAMESPACE_END

#elif defined(VTK_USE_EXTERN_TEMPLATE)

#ifndef VTK_STRIDED_BACKEND_TEMPLATE_EXTERN
#define VTK_STRIDED_BACKEND_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkStridedImplicitBackend);
VTK_ABI_NAMESPACE_END
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_STRIDED_IMPLICIT_BACKEND_TEMPLATE_EXTERN

#endif
