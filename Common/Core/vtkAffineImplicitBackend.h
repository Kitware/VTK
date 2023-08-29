// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkAffineImplicitBackend_h
#define vtkAffineImplicitBackend_h

#include "vtkCommonCoreModule.h"

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
struct VTKCOMMONCORE_EXPORT vtkAffineImplicitBackend final
{
  /**
   * A non-trivially constructible constructor
   *
   * \param slope the slope of the affine function
   * \param intercept the intercept value at the origin (i.e. the value at 0)
   */
  vtkAffineImplicitBackend(ValueType slope, ValueType intercept);

  /**
   * The main call method for the backend
   *
   * \param index the index at which one wished to evaluate the backend
   * \return the affinely computed value
   */
  ValueType operator()(int index) const;

  /**
   * The slope of the affine function on the indeces
   */
  ValueType Slope;
  /**
   * The value of the affine function at index 0
   */
  ValueType Intercept;
};
VTK_ABI_NAMESPACE_END

#endif // vtkAffineImplicitBackend_h
