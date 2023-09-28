// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkAffineImplicitBackend_txx
#define vtkAffineImplicitBackend_txx

#include "vtkAffineImplicitBackend.h"

/**
 * \struct vtkAffineImplicitBackend
 * \brief A utility structure serving as a backend for affine (as a function of the index) implicit
 * arrays
 *
 * This structure can be classified as a closure and can be called using syntax similar to a
 * function call.
 *
 * At construction it takes two parameters: the slope of the map and the intercept. It returns a
 * value calculated as:
 *
 *   value = slope * index + intercept
 *
 * An example of potential usage in a vtkImplicitArray
 * ```
 * double slope = some_number;
 * double intercept = some_other_number;
 * vtkNew<vtkImplicitArray<vtkAffineImplicitBackend<double>>> affineArray;
 * affineArray->SetBackend(std::make_shared<vtkAffineImplicitBackend<double>>(slope, intercept));
 * affineArray->SetNumberOfTuples(however_many_you_want);
 * affineArray->SetNumberOfComponents(whatever_youd_like);
 * double value = affineArray->GetTypedComponent(index_in_tuple_range, index_in_component_range);
 * ```
 */
VTK_ABI_NAMESPACE_BEGIN
template <typename ValueType>
vtkAffineImplicitBackend<ValueType>::vtkAffineImplicitBackend(ValueType slope, ValueType intercept)
  : Slope(slope)
  , Intercept(intercept)
{
}

template <typename ValueType>
ValueType vtkAffineImplicitBackend<ValueType>::operator()(int index) const
{
  return this->Slope * static_cast<ValueType>(index) + this->Intercept;
}
VTK_ABI_NAMESPACE_END

#endif // vtkAffineImplicitBackend_txx

#if defined(VTK_AFFINE_BACKEND_INSTANTIATING)

#define VTK_INSTANTIATE_AFFINE_BACKEND(ValueType)                                                  \
  VTK_ABI_NAMESPACE_BEGIN                                                                          \
  template struct VTKCOMMONCORE_EXPORT vtkAffineImplicitBackend<ValueType>;                        \
  VTK_ABI_NAMESPACE_END

#elif defined(VTK_USE_EXTERN_TEMPLATE)

#ifndef VTK_AFFINE_BACKEND_TEMPLATE_EXTERN
#define VTK_AFFINE_BACKEND_TEMPLATE_EXTERN
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4910) // extern and dllexport incompatible
#endif
VTK_ABI_NAMESPACE_BEGIN
vtkExternTemplateMacro(extern template struct VTKCOMMONCORE_EXPORT vtkAffineImplicitBackend);
VTK_ABI_NAMESPACE_END
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // VTK_AFFINE_IMPLICIT_BACKEND_TEMPLATE_EXTERN

#endif
