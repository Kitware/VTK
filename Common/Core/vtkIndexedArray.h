// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
/**
 * \class vtkIndexedArray
 * \brief A utility array for creating a wrapper array around an existing array and reindexing its
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

#ifndef vtkIndexedArray_h
#define vtkIndexedArray_h

#include "vtkCommonCoreModule.h" // for export macro
#include "vtkCompiler.h"         // for VTK_USE_EXTERN_TEMPLATE
#include "vtkImplicitArray.h"
#include "vtkIndexedImplicitBackend.h" // for the array backend

VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkIndexedArray
#ifndef __VTK_WRAP__
  : public vtkImplicitArray<vtkIndexedImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_INDEXED_ARRAY>
{
  using ImplicitArrayType =
    vtkImplicitArray<vtkIndexedImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_INDEXED_ARRAY>;
#else // Fake the superclass for the wrappers.
  : public vtkDataArray
{
  using ImplicitArrayType = vtkDataArray;
#endif
public:
  using SelfType = vtkIndexedArray<ValueTypeT>;
  vtkImplicitArrayTypeMacro(SelfType, ImplicitArrayType);
#ifndef __VTK_WRAP__
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;
#else
  using ValueType = ValueTypeT;
#endif

  static vtkIndexedArray* New();

  // This macro expands to the set of method declarations that
  // make up the interface of vtkImplicitArray, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateImplicitWrappedArrayInterface(ValueTypeT);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkIndexedArray<ValueType>* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkIndexedArray<ValueType>*>(Superclass::FastDownCast(source));
  }

  ///@{
  /**
   * Set which indexes from array should be exposed.
   */
  void ConstructBackend(vtkIdList* indexes, vtkDataArray* array);
  void ConstructBackend(vtkDataArray* indexes, vtkDataArray* array);
  ///@}

  /**
   * Get the original base array used for value lookup.
   */
  vtkDataArray* GetBaseArray();

  /**
   * Get the original index array used for indirection.
   */
  vtkDataArray* GetIndexArray();

protected:
  vtkIndexedArray() = default;
  ~vtkIndexedArray() override = default;

private:
  vtkIndexedArray(const vtkIndexedArray&) = delete;
  void operator=(const vtkIndexedArray&) = delete;
};

// Declare vtkArrayDownCast implementations for Indexed arrays:
vtkArrayDownCast_TemplateFastCastMacro(vtkIndexedArray);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkIndexedArray.
#define vtkCreateIndexedWrappedArrayInterface(T)                                                   \
  vtkCreateImplicitWrappedArrayInterface(T);                                                       \
  void ConstructBackend(vtkIdList* indexes, vtkDataArray* array);                                  \
  void ConstructBackend(vtkDataArray* indexes, vtkDataArray* array);                               \
  vtkDataArray* GetBaseArray();                                                                    \
  vtkDataArray* GetIndexArray();

#endif // vtkIndexedArray_h

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkIndexedArray can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_INDEXED_ARRAY_INSTANTIATING
#define VTK_INDEXED_ARRAY_INSTANTIATE(T)                                                           \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<T>, double);                                \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkIndexedArray<T>;                                          \
  VTK_ABI_NAMESPACE_END
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#define VTK_INDEXED_ARRAY_INSTANTIATE_VALUERANGE(T)                                                \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<T>, T);                                     \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_INDEXED_ARRAY_EXTERN
#define VTK_INDEXED_ARRAY_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkIndexedArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkIndexedArray);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<long>, long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned long>, unsigned long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<long long>, long long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned long long>, unsigned long long)
// These are instantiated by vtkIndexedArrayInstantiate_double.cxx.inc, e.t.c.
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkIndexedArray<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_INDEXED_ARRAY_EXTERN

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
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkIndexedArray);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkIndexedArray.h
