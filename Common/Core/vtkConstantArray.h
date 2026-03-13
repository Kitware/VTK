// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
/**
 * \class vtkConstantArray
 * \brief A utility array for wrapping constant functions in implicit arrays
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time.
 *
 * @sa
 * vtkImplicitArray vtkConstantImplicitBackend
 */

#ifndef vtkConstantArray_h
#define vtkConstantArray_h

#include "vtkCommonCoreModule.h"        // for export macro
#include "vtkCompiler.h"                // for VTK_USE_EXTERN_TEMPLATE
#include "vtkConstantImplicitBackend.h" // for the array backend
#include "vtkImplicitArray.h"

VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkConstantArray
#ifndef __VTK_WRAP__
  : public vtkImplicitArray<vtkConstantImplicitBackend<ValueTypeT>,
      vtkArrayTypes::VTK_CONSTANT_ARRAY>
{
  using ImplicitArrayType =
    vtkImplicitArray<vtkConstantImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_CONSTANT_ARRAY>;
#else // Fake the superclass for the wrappers.
  : public vtkDataArray
{
  using ImplicitArrayType = vtkDataArray;
#endif
public:
  using SelfType = vtkConstantArray<ValueTypeT>;
  vtkImplicitArrayTypeMacro(SelfType, ImplicitArrayType);
#ifndef __VTK_WRAP__
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;
#else
  using ValueType = ValueTypeT;
#endif

  static vtkConstantArray* New();

  // This macro expands to the set of method declarations that
  // make up the interface of vtkImplicitArray, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateImplicitWrappedArrayInterface(ValueTypeT);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkConstantArray<ValueType>* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkConstantArray<ValueType>*>(Superclass::FastDownCast(source));
  }

  /**
   * Set the constant value for this array.
   */
  void ConstructBackend(ValueType value);

  /**
   * Get the constant value stored in this array.
   */
  ValueType GetConstantValue() const
  {
    return const_cast<vtkConstantArray<ValueType>*>(this)->GetBackend()->Value;
  }

  /**
   * Check whether the backend has been constructed.
   */
  bool IsBackendConstructed() const
  {
    return const_cast<vtkConstantArray<ValueType>*>(this)->GetBackend() != nullptr;
  }

protected:
  vtkConstantArray() = default;
  ~vtkConstantArray() override = default;

private:
  vtkConstantArray(const vtkConstantArray&) = delete;
  void operator=(const vtkConstantArray&) = delete;
};

// Declare vtkArrayDownCast implementations for Constant arrays:
vtkArrayDownCast_TemplateFastCastMacro(vtkConstantArray);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkConstantArray.
#define vtkCreateConstantWrappedArrayInterface(T)                                                  \
  vtkCreateImplicitWrappedArrayInterface(T);                                                       \
  void ConstructBackend(T value);                                                                  \
  T GetConstantValue() const;                                                                      \
  bool IsBackendConstructed() const;

#endif // vtkConstantArray_h

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkConstantArray can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_CONSTANT_ARRAY_INSTANTIATING
#define VTK_CONSTANT_ARRAY_INSTANTIATE(T)                                                          \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkConstantArray<T>, double);                               \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkConstantArray<T>;                                         \
  VTK_ABI_NAMESPACE_END
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#define VTK_CONSTANT_ARRAY_INSTANTIATE_VALUERANGE(T)                                               \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkConstantArray<T>, T);                                    \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_CONSTANT_ARRAY_EXTERN
#define VTK_CONSTANT_ARRAY_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkConstantArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkConstantArray);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<long>, long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned long>, unsigned long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<long long>, long long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned long long>, unsigned long long)
// These are instantiated by vtkConstantArrayInstantiate_double.cxx.inc, e.t.c.
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkConstantArray<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_CONSTANT_ARRAY_EXTERN

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
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkConstantArray);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkConstantArray.h
