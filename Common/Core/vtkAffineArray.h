// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
/**
 * \class vtkAffineArray
 * \brief A utility array for wrapping affine functions in implicit arrays
 *
 * In order to be usefully included in the dispatchers, these arrays need to be instantiated at the
 * vtk library compile time.
 *
 * @sa
 * vtkImplicitArray vtkAffineImplicitBackend
 */
#ifndef vtkAffineArray_h
#define vtkAffineArray_h

#include "vtkAffineImplicitBackend.h" // for the array backend
#include "vtkCommonCoreModule.h"      // for export macro
#include "vtkCompiler.h"              // for VTK_USE_EXTERN_TEMPLATE
#include "vtkImplicitArray.h"

VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkAffineArray
#ifndef __VTK_WRAP__
  : public vtkImplicitArray<vtkAffineImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_AFFINE_ARRAY>
{
  using ImplicitArrayType =
    vtkImplicitArray<vtkAffineImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_AFFINE_ARRAY>;
#else // Fake the superclass for the wrappers.
  : public vtkDataArray
{
  using ImplicitArrayType = vtkDataArray;
#endif
public:
  using SelfType = vtkAffineArray<ValueTypeT>;
  vtkImplicitArrayTypeMacro(SelfType, ImplicitArrayType);
#ifndef __VTK_WRAP__
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;
#else
  using ValueType = ValueTypeT;
#endif

  static vtkAffineArray* New();

  // This macro expands to the set of method declarations that
  // make up the interface of vtkImplicitArray, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateImplicitWrappedArrayInterface(ValueTypeT);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkAffineArray<ValueType>* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkAffineArray<ValueType>*>(Superclass::FastDownCast(source));
  }

  /**
   * Set the parameters for the affine backend.
   * slope is the unit variation and intercept is the value at 0.
   * Value at index is:
   *   value = slope * index + intercept
   */
  void ConstructBackend(ValueType slope, ValueType intercept);

  /**
   * Get the slope of the affine function.
   */
  ValueType GetSlope() const
  {
    return const_cast<vtkAffineArray<ValueType>*>(this)->GetBackend()->Slope;
  }

  /**
   * Get the intercept of the affine function.
   */
  ValueType GetIntercept() const
  {
    return const_cast<vtkAffineArray<ValueType>*>(this)->GetBackend()->Intercept;
  }

  /**
   * Check whether the backend has been constructed.
   */
  bool IsBackendConstructed() const
  {
    return const_cast<vtkAffineArray<ValueType>*>(this)->GetBackend() != nullptr;
  }

protected:
  vtkAffineArray() = default;
  ~vtkAffineArray() override = default;

private:
  vtkAffineArray(const vtkAffineArray&) = delete;
  void operator=(const vtkAffineArray&) = delete;
};

// Declare vtkArrayDownCast implementations for Affine arrays:
vtkArrayDownCast_TemplateFastCastMacro(vtkAffineArray);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkAffineArray.
#define vtkCreateAffineWrappedArrayInterface(T)                                                    \
  vtkCreateImplicitWrappedArrayInterface(T);                                                       \
  void ConstructBackend(T slope, T intercept);                                                     \
  T GetSlope() const;                                                                              \
  T GetIntercept() const;                                                                          \
  bool IsBackendConstructed() const;

#endif // vtkAffineArray_h

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkAffineArray can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_AFFINE_ARRAY_INSTANTIATING
#define VTK_AFFINE_ARRAY_INSTANTIATE(T)                                                            \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkAffineArray<T>, double);                                 \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkAffineArray<T>;                                           \
  VTK_ABI_NAMESPACE_END
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#define VTK_AFFINE_ARRAY_INSTANTIATE_VALUERANGE(T)                                                 \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkAffineArray<T>, T);                                      \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_AFFINE_ARRAY_EXTERN
#define VTK_AFFINE_ARRAY_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkAffineArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkAffineArray);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN
// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<long>, long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned long>, unsigned long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<long long>, long long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned long long>, unsigned long long)
// These are instantiated by vtkAffineArrayInstantiate_double.cxx.inc, e.t.c.
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkAffineArray<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_AFFINE_ARRAY_EXTERN

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
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkAffineArray);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkAffineArray.h
