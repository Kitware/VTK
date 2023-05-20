// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkStructuredPointArray_h
#define vtkStructuredPointArray_h

#ifdef VTK_STRUCTURED_POINT_ARRAY_INSTANTIATING
#define VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#include "vtkDataArrayPrivate.txx"
#endif

#include "vtkCommonCoreModule.h"       // For export macro
#include "vtkImplicitArray.h"          // For vtkImplicitArray
#include "vtkSmartPointer.h"           // For vtkSmartPointer
#include "vtkStructuredPointBackend.h" // For vtkStructuredPointBackend

#ifdef VTK_STRUCTURED_POINT_ARRAY_INSTANTIATING
#undef VTK_IMPLICIT_VALUERANGE_INSTANTIATING
#endif

/**
 * @class   vtkStructuredPointArray
 * @brief   An structured point array used by structured datasets subclasses.
 *
 * This class is used by structured datasets subclasses to represent
 * the implicit function of the structured point array. It is templated over the
 * type of the point array.
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
using vtkStructuredPointArray = vtkImplicitArray<vtkStructuredPointBackend<ValueType>>;
VTK_ABI_NAMESPACE_END

namespace vtk
{
VTK_ABI_NAMESPACE_BEGIN
/**
 * @brief Create an implicit point array from the given coordinates and direction matrix
 * which is optional. xCoords, yCoords and zCoords are the coordinates of the points.
 * extent is the extent of the dataset. dataDescription is the data description of the dataset.
 * dirMatrix is the direction matrix of the dataset (if any, else provide a homogeneous matrix).
 */
template <typename ValueType>
vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>>> CreateStructuredPointArray(
  vtkDataArray* xCoords, vtkDataArray* yCoords, vtkDataArray* zCoords, int extent[6],
  int dataDescription, double dirMatrix[9]);
VTK_ABI_NAMESPACE_END
}

#endif // vtkStructuredPointArray_h

#ifdef VTK_STRUCTURED_POINT_ARRAY_INSTANTIATING
// The instantiation is separated in two functions because the .txx includes vtkArrayDispatch.h
// which when Dispatching is enabled, it instantiates a class with a value type, before exporting it
#define VTK_INSTANTIATE_STRUCTURED_POINT_ARRAY_EXPORT(ValueType)                                   \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template class VTKCOMMONCORE_EXPORT vtkImplicitArray<vtkStructuredPointBackend<ValueType>>;      \
  VTK_ABI_NAMESPACE_END

#define VTK_INSTANTIATE_STRUCTURED_POINT_ARRAY_FUNCTIONS(ValueType)                                \
  namespace vtk                                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template VTKCOMMONCORE_EXPORT                                                                    \
    vtkSmartPointer<vtkImplicitArray<vtkStructuredPointBackend<ValueType>>>                        \
    CreateStructuredPointArray(vtkDataArray* xCoords, vtkDataArray* yCoords,                       \
      vtkDataArray* zCoords, int extent[6], int dataDescription, double dirMatrix[9]);             \
  VTK_ABI_NAMESPACE_END                                                                            \
  }                                                                                                \
  namespace vtkDataArrayPrivate                                                                    \
  {                                                                                                \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  VTK_INSTANTIATE_VALUERANGE_ARRAYTYPE(                                                            \
    vtkImplicitArray<vtkStructuredPointBackend<ValueType>>, double)                                \
  VTK_ABI_NAMESPACE_END                                                                            \
  }

#elif defined(VTK_USE_EXTERN_TEMPLATE)
#ifndef VTK_STRUCTURED_POINT_ARRAY_TEMPLATE_EXTERN
#define VTK_STRUCTURED_POINT_ARRAY_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
// The following is needed when the vtkCompositeArray is declared
// dllexport and is used from another class in vtkCommonCore
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternSecondOrderTemplateMacro(
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkStructuredPointBackend);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
VTK_ABI_NAMESPACE_END
#endif // VTK_STRUCTURED_POINT_ARRAY_TEMPLATE_EXTERN
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
  extern template class VTKCOMMONCORE_EXPORT vtkImplicitArray, vtkStructuredPointBackend);

#pragma warning(pop)

VTK_ABI_NAMESPACE_END
#endif
