// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
/**
 * \class vtkCompositeArray
 * \brief A utility array for concatenating arrays into an implicit array
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

#ifndef vtkCompositeArray_h
#define vtkCompositeArray_h

#include "vtkCommonCoreModule.h"         // for export macro
#include "vtkCompiler.h"                 // for VTK_USE_EXTERN_TEMPLATE
#include "vtkCompositeImplicitBackend.h" // for the array backend
#include "vtkImplicitArray.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkCompositeArray
#ifndef __VTK_WRAP__
  : public vtkImplicitArray<vtkCompositeImplicitBackend<ValueTypeT>,
      vtkArrayTypes::VTK_COMPOSITE_ARRAY>
{
  using ImplicitArrayType =
    vtkImplicitArray<vtkCompositeImplicitBackend<ValueTypeT>, vtkArrayTypes::VTK_COMPOSITE_ARRAY>;
#else // Fake the superclass for the wrappers.
  : public vtkDataArray
{
  using ImplicitArrayType = vtkDataArray;
#endif
public:
  using SelfType = vtkCompositeArray<ValueTypeT>;
  vtkImplicitArrayTypeMacro(SelfType, ImplicitArrayType);
#ifndef __VTK_WRAP__
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;
#else
  using ValueType = ValueTypeT;
#endif

  static vtkCompositeArray* New();

  // This macro expands to the set of method declarations that
  // make up the interface of vtkImplicitArray, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateImplicitWrappedArrayInterface(ValueTypeT);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkCompositeArray<ValueType>* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkCompositeArray<ValueType>*>(Superclass::FastDownCast(source));
  }

  /**
   * Set the underlying arrays to use.
   */
  void ConstructBackend(vtkDataArrayCollection* arrays);

  /**
   * Get the number of original arrays composing this composite array.
   */
  vtkIdType GetNumberOfArrays();

  /**
   * Get the original array at the given index.
   */
  vtkDataArray* GetArray(vtkIdType idx);

  /**
   * Get the tuple offset of the array at the given index.
   */
  vtkIdType GetOffset(vtkIdType idx);

protected:
  vtkCompositeArray() = default;
  ~vtkCompositeArray() override = default;

private:
  vtkCompositeArray(const vtkCompositeArray&) = delete;
  void operator=(const vtkCompositeArray&) = delete;
};

// Declare vtkArrayDownCast implementations for Composite arrays:
vtkArrayDownCast_TemplateFastCastMacro(vtkCompositeArray);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkCompositeArray.
#define vtkCreateCompositeWrappedArrayInterface(T)                                                 \
  vtkCreateImplicitWrappedArrayInterface(T);                                                       \
  void ConstructBackend(vtkDataArrayCollection* arrays);                                           \
  vtkIdType GetNumberOfArrays();                                                                   \
  vtkDataArray* GetArray(vtkIdType idx);                                                           \
  vtkIdType GetOffset(vtkIdType idx);

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
/**
 * \fn ConcatenateDataArrays
 * A method that can take a `std::vector` of `vtkDataArray`s and concatenate them together into a
 * single `vtkCompositeArray`. Input arrays should all have the same number of components and the
 * resulting composite array has as many tuples as the sum of all the inputs.
 *
 * The method is templated based on the value type of composite array the caller wishes as a result.
 */
template <typename ValueTypeT>
vtkSmartPointer<vtkCompositeArray<ValueTypeT>> ConcatenateDataArrays(
  const std::vector<vtkDataArray*>& arrays);
VTK_ABI_NAMESPACE_END
}

#endif // vtkCompositeArray_h

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkCompositeArray can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_COMPOSITE_ARRAY_INSTANTIATING
#define VTK_COMPOSITE_ARRAY_INSTANTIATE(T)                                                         \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<T>, double);                              \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkCompositeArray<T>;                                        \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtk                                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template VTKCOMMONCORE_EXPORT vtkSmartPointer<vtkCompositeArray<T>> ConcatenateDataArrays(       \
    const std::vector<vtkDataArray*>& arrays);                                                     \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#define VTK_COMPOSITE_ARRAY_INSTANTIATE_VALUERANGE(T)                                              \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<T>, T);                                   \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_COMPOSITE_ARRAY_EXTERN
#define VTK_COMPOSITE_ARRAY_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkCompositeArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkCompositeArray);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<long>, long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned long>, unsigned long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<long long>, long long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned long long>, unsigned long long)
// These are instantiated by vtkCompositeArrayInstantiate_double.cxx.inc, e.t.c.
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkCompositeArray<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_COMPOSITE_ARRAY_EXTERN

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
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkCompositeArray);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkCompositeArray.h
