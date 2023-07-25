// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmGradient
 * @brief   A general filter for gradient estimation.
 *
 * Estimates the gradient of a field in a data set.  The gradient calculation
 * is dependent on the input dataset type.  The created gradient array
 * is of the same type as the array it is calculated from (e.g. point data
 * or cell data) as well as data type (e.g. float, double). The output array has
 * 3*number of components of the input data array.  The ordering for the
 * output tuple will be {du/dx, du/dy, du/dz, dv/dx, dv/dy, dv/dz, dw/dx,
 * dw/dy, dw/dz} for an input array {u, v, w}.
 *
 * Also options to additionally compute the divergence, vorticity and
 * Q criterion of input vector fields.
 *
 */

#ifndef vtkmGradient_h
#define vtkmGradient_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkGradientFilter.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmGradient : public vtkGradientFilter
{
public:
  vtkTypeMacro(vtkmGradient, vtkGradientFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmGradient* New();

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if VTK-m fails to run. When the flag is on,
   * the filter will generate an error if VTK-m fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, vtkTypeBool);
  vtkSetMacro(ForceVTKm, vtkTypeBool);
  vtkBooleanMacro(ForceVTKm, vtkTypeBool);
  ///@}

protected:
  /// \brief Check if the input dataset and parameters combination is supported by this filter
  ///
  /// Certain input and parameters combinations are not currently supported by vtkm.
  /// This information is internally used to determine if this filter should fall back to
  /// Superclass implementation.
  bool CanProcessInput(vtkDataSet* input);

  vtkmGradient();
  ~vtkmGradient() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ForceVTKm = false;

private:
  vtkmGradient(const vtkmGradient&) = delete;
  void operator=(const vtkmGradient&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmGradient_h
