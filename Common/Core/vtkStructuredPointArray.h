// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredPointArray
 * @brief   A structured point array used by vtkCartesianGrid subclasses.
 *
 * This class is used by vtkCartesianGrid subclasses to represent
 * the implicit function of the structured point array. It is templated over the
 * type of the point array.
 *
 * @sa
 * vtkImplicitArray vtkStructuredPointBackend
 */

#ifndef vtkStructuredPointArray_h
#define vtkStructuredPointArray_h

#include "vtkCommonCoreModule.h"       // For export macro
#include "vtkCompiler.h"               // for VTK_USE_EXTERN_TEMPLATE
#include "vtkImplicitArray.h"          // For vtkImplicitArray
#include "vtkSmartPointer.h"           // For vtkSmartPointer
#include "vtkStructuredPointBackend.h" // For vtkStructuredPointBackend

VTK_ABI_NAMESPACE_BEGIN
template <class ValueTypeT>
class VTKCOMMONCORE_EXPORT vtkStructuredPointArray
#ifndef __VTK_WRAP__
  : public vtkImplicitArray<vtkStructuredPointBackend<ValueTypeT>,
      vtkArrayTypes::VTK_STRUCTURED_POINT_ARRAY>
{
  using ImplicitArrayType = vtkImplicitArray<vtkStructuredPointBackend<ValueTypeT>,
    vtkArrayTypes::VTK_STRUCTURED_POINT_ARRAY>;
#else // Fake the superclass for the wrappers.
  : public vtkDataArray
{
  using ImplicitArrayType = vtkDataArray;
#endif
public:
  using SelfType = vtkStructuredPointArray<ValueTypeT>;
  vtkImplicitArrayTypeMacro(SelfType, ImplicitArrayType);
#ifndef __VTK_WRAP__
  using typename Superclass::ArrayTypeTag;
  using typename Superclass::DataTypeTag;
  using typename Superclass::ValueType;
#else
  using ValueType = ValueTypeT;
#endif

  static vtkStructuredPointArray* New();

  // This macro expands to the set of method declarations that
  // make up the interface of vtkImplicitArray, which is ignored
  // by the wrappers.
#if defined(__VTK_WRAP__) || defined(__WRAP_GCCXML__)
  vtkCreateImplicitWrappedArrayInterface(ValueTypeT);
#endif

  /**
   * A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
   */
  static vtkStructuredPointArray<ValueType>* FastDownCast(vtkAbstractArray* source)
  {
    return static_cast<vtkStructuredPointArray<ValueType>*>(Superclass::FastDownCast(source));
  }

  ///@{
  /**
   * Set the parameters for the strided backend.
   */
  void ConstructBackend(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,
    int extent[6], int dataDescription, double dirMatrix[9]);
  void ConstructBackend(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,
    int extent[6], int dataDescription);
  ///@}

  ///@{
  /**
   * Get the coordinate arrays used to construct the backend.
   */
  vtkDataArray* GetXCoordinates();
  vtkDataArray* GetYCoordinates();
  vtkDataArray* GetZCoordinates();
  ///@}

  /**
   * Return true if a non-identity direction matrix is being used.
   */
  bool GetUsesDirectionMatrix();

protected:
  vtkStructuredPointArray() = default;
  ~vtkStructuredPointArray() override = default;

private:
  vtkStructuredPointArray(const vtkStructuredPointArray&) = delete;
  void operator=(const vtkStructuredPointArray&) = delete;
};

// Declare vtkArrayDownCast implementations for StructuredPoint arrays:
vtkArrayDownCast_TemplateFastCastMacro(vtkStructuredPointArray);

VTK_ABI_NAMESPACE_END

// This macro is used by the subclasses to create dummy
// declarations for these functions such that the wrapper
// can see them. The wrappers ignore vtkStructuredPointArray.
#define vtkCreateStructuredPointWrappedArrayInterface(T)                                           \
  vtkCreateImplicitWrappedArrayInterface(T);                                                       \
  void ConstructBackend(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,       \
    int extent[6], int dataDescription, double dirMatrix[9]);                                      \
  void ConstructBackend(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,       \
    int extent[6], int dataDescription);                                                           \
  vtkDataArray* GetXCoordinates();                                                                 \
  vtkDataArray* GetYCoordinates();                                                                 \
  vtkDataArray* GetZCoordinates();                                                                 \
  bool GetUsesDirectionMatrix();

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
/**
 * @brief Create an implicit point array from the given coordinates and direction matrix
 * which is optional. xCoords, yCoords and zCoords are the coordinates of the points.
 * extent is the extent of the dataset. dataDescription is the data description of the dataset.
 * dirMatrix is the direction matrix of the dataset (if any, else provide a homogeneous matrix).
 */
template <typename ValueTypeT>
vtkSmartPointer<vtkStructuredPointArray<ValueTypeT>> CreateStructuredPointArray(
  vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords, int extent[6],
  int dataDescription, double dirMatrix[9]);
VTK_ABI_NAMESPACE_END
}

#endif // vtkStructuredPointArray_h

// This portion must be OUTSIDE the include blockers. This is used to tell
// libraries other than vtkCommonCore that instantiations of
// vtkStructuredPointArray can be found externally. This prevents each library
// from instantiating these on their own.
#ifdef VTK_STRUCTURED_POINT_ARRAY_INSTANTIATING
#define VTK_STRUCTURED_POINT_ARRAY_INSTANTIATE(T)                                                  \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<T>, double);                        \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkStructuredPointArray<T>;                                  \
  VTK_ABI_NAMESPACE_END                                                                            \
  namespace vtk                                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template VTKCOMMONCORE_EXPORT vtkSmartPointer<vtkStructuredPointArray<T>>                        \
  CreateStructuredPointArray(vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords,  \
    int extent[6], int dataDescription, double dirMatrix[9]);                                      \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
// We only provide these specializations for the 64-bit integer types, since
// other types can reuse the double-precision mechanism in
// vtkDataArray::GetRange without losing precision.
#define VTK_STRUCTURED_POINT_ARRAY_INSTANTIATE_VALUERANGE(T)                                       \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<T>, T);                             \
  VTK_ABI_NAMESPACE_END                                                                            \
  }
#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_STRUCTURED_POINT_ARRAY_EXTERN
#define VTK_STRUCTURED_POINT_ARRAY_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkStructuredPointArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkStructuredPointArray);
VTK_ABI_NAMESPACE_END

namespace vtkDataArrayPrivate
{
VTK_ABI_NAMESPACE_BEGIN

// These are instantiated in vtkGenericDataArrayValueRange${i}.cxx
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<long>, long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned long>, unsigned long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<long long>, long long)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned long long>, unsigned long long)
// These are instantiated by vtkStructuredPointArrayInstantiate_double.cxx.inc, e.t.c.
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<float>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<double>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<signed char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned char>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned short>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned int>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<long long>, double)
VTK_DECLARE_VALUERANGE_ARRAYTYPE(vtkStructuredPointArray<unsigned long long>, double)

VTK_ABI_NAMESPACE_END
} // namespace vtkDataArrayPrivate

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_STRUCTURED_POINT_ARRAY_EXTERN

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
vtkInstantiateTemplateMacro(extern template class VTKCOMMONCORE_EXPORT vtkStructuredPointArray);
VTK_ABI_NAMESPACE_END

#pragma warning(pop)

#endif

// VTK-HeaderTest-Exclude: vtkStructuredPointArray.h
