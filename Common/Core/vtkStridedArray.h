// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * \class vtkStridedArray
 * \brief A utility array for wrapping a strided buffer in implicit arrays
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

#ifndef vtkStridedArray_h
#define vtkStridedArray_h

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkCompiler.h"         // for VTK_USE_EXTERN_TEMPLATE
#include "vtkImplicitArray.h"
#include "vtkSmartPointer.h"           // for vtkSmartPointer
#include "vtkStridedImplicitBackend.h" // for the array backend

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractBuffer;
VTK_ABI_NAMESPACE_END

VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkStridedArray
#ifndef __VTK_WRAP__
  : public vtkImplicitArray<vtkStridedImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_STRIDED_ARRAY>
{
  using ImplicitArrayType =
    vtkImplicitArray<vtkStridedImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_STRIDED_ARRAY>;
#else // Fake the superclass for the wrappers.
  : public vtkDataArray
{
  using ImplicitArrayType = vtkDataArray;
#endif
public:
  using SelfType = vtkStridedArray<ValueTypeT>;
  vtkImplicitArrayTypeMacro(SelfType, ImplicitArrayType);
#ifndef __VTK_WRAP__
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;
#else
  using ValueType = ValueTypeT;
#endif

  static vtkStridedArray* New();

  // This macro expands to the set of method declarations that
  // make up the interface of vtkImplicitArray, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateImplicitWrappedArrayInterface(ValueTypeT);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkStridedArray<ValueType>* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkStridedArray<ValueType>*>(Superclass::FastDownCast(source));
  }

  ///@{
  /**
   * Set the parameters for the strided backend using a vtkBuffer (via
   * vtkAbstractBuffer) as the buffer source.  The buffer is stored with
   * reference counting to keep the memory alive.  This is the preferred
   * overload when constructing from Python.
   */
  void ConstructBackend(
    vtkAbstractBuffer* buffer, vtkIdType stride, int components, vtkIdType offset);
  void ConstructBackend(vtkAbstractBuffer* buffer, vtkIdType stride, int components);
  void ConstructBackend(vtkAbstractBuffer* buffer, vtkIdType stride);
  ///@}

  ///@{
  /**
   * Set the parameters for the strided backend from a raw pointer.
   * @warning The buffer is not owned — the caller must ensure the memory
   * outlives the vtkStridedArray.
   */
  void ConstructBackend(
    const ValueType* buffer, vtkIdType stride, int components, vtkIdType offset);
  void ConstructBackend(const ValueType* buffer, vtkIdType stride, int components);
  void ConstructBackend(const ValueType* buffer, vtkIdType stride);
  ///@}

  /**
   * Get the buffer source object, if one was provided via the
   * vtkAbstractBuffer overload of ConstructBackend.  Returns nullptr
   * if the array was constructed from a raw pointer.
   */
  vtkAbstractBuffer* GetBufferSource() const { return this->BufferSource; }

  /**
   * Get the buffer of the strided backend.
   */
  const ValueType* GetBuffer() const
  {
    return const_cast<vtkStridedArray<ValueType>*>(this)->GetBackend()->GetBuffer();
  }

  /**
   * Get the stride of the strided backend.
   */
  vtkIdType GetStride() const
  {
    return const_cast<vtkStridedArray<ValueType>*>(this)->GetBackend()->GetStride();
  }

  /**
   * Get the offset of the strided backend.
   */
  vtkIdType GetOffset() const
  {
    return const_cast<vtkStridedArray<ValueType>*>(this)->GetBackend()->GetOffset();
  }

protected:
  vtkStridedArray() = default;
  ~vtkStridedArray() override = default;

private:
  vtkStridedArray(const vtkStridedArray&) = delete;
  void operator=(const vtkStridedArray&) = delete;

  /**
   * Stored reference to the buffer source, if constructed via the
   * vtkAbstractBuffer overload.  Keeps the buffer memory alive.
   */
  vtkSmartPointer<vtkBuffer<ValueType>> BufferSource;
};

// Declare vtkArrayDownCast implementations for STRIDED arrays:
vtkArrayDownCast_TemplateFastCastMacro(vtkStridedArray);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkStridedArray.
#define vtkCreateStridedWrappedArrayInterface(T)                                                   \
  vtkCreateImplicitWrappedArrayInterface(T);                                                       \
  void ConstructBackend(                                                                           \
    vtkAbstractBuffer* buffer, vtkIdType stride, int components, vtkIdType offset);                \
  void ConstructBackend(vtkAbstractBuffer* buffer, vtkIdType stride, int components);              \
  void ConstructBackend(vtkAbstractBuffer* buffer, vtkIdType stride);                              \
  void ConstructBackend(const T* buffer, vtkIdType stride, int components, vtkIdType offset);      \
  void ConstructBackend(const T* buffer, vtkIdType stride, int components);                        \
  void ConstructBackend(const T* buffer, vtkIdType stride);                                        \
  const T* GetBuffer() const;                                                                      \
  vtkAbstractBuffer* GetBufferSource() const;                                                      \
  vtkIdType GetStride() const;                                                                     \
  vtkIdType GetOffset() const;

#endif // vtkStridedArray_h

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkStridedArray can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_STRIDED_ARRAY_INSTANTIATING
#define VTK_STRIDED_ARRAY_INSTANTIATE(T)                                                           \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkStridedArray<T>, double);                                \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkStridedArray<T>;                                          \
  VTK_ABI_NAMESPACE_END
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#define VTK_STRIDED_ARRAY_INSTANTIATE_VALUERANGE(T)                                                \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkStridedArray<T>, T);                                     \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_STRIDED_ARRAY_EXTERN
#define VTK_STRIDED_ARRAY_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkStridedArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkStridedArray);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<long>, long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned long>, unsigned long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<long long>, long long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned long long>, unsigned long long)
// These are instantiated by vtkStridedArrayInstantiate_double.cxx.inc, e.t.c.
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStridedArray<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_STRIDED_ARRAY_EXTERN

// The following clause is only for MSVC
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
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkStridedArray);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkStridedArray.h
