// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class vtkmClip
 * @brief Clip a dataset using the accelerated vtk-m Clip filter.
 *
 * Clip a dataset using either a given value or by using an vtkImplicitFunction
 * Currently the supported implicit functions are Box, Plane, and Sphere.
 *
 */

#ifndef vtkmClip_h
#define vtkmClip_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // For export macro
#include "vtkDeprecation.h"                   // For VTK_DEPRECATED_IN_9_3_0
#include "vtkTableBasedClipDataSet.h"

#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

#include <memory> // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmClip : public vtkTableBasedClipDataSet
{
public:
  static vtkmClip* New();
  vtkTypeMacro(vtkmClip, vtkTableBasedClipDataSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The scalar value to use when clipping the dataset. Values greater than
   * ClipValue are preserved in the output dataset. Default is 0.
   */
  VTK_DEPRECATED_IN_9_3_0("Please use GetValue instead.")
  double GetClipValue() { return this->GetValue(); }

  VTK_DEPRECATED_IN_9_3_0("Please use SetValue instead.")
  void SetClipValue(double v) { this->SetValue(v); }

  /**
   * If true, all input point data arrays will be mapped onto the output
   * dataset. Default is true.
   */
  vtkGetMacro(ComputeScalars, bool);
  vtkSetMacro(ComputeScalars, bool);
  vtkBooleanMacro(ComputeScalars, bool);

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
  vtkmClip();
  ~vtkmClip() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ForceVTKm = false;
  bool ComputeScalars = true;

  struct internals;

private:
  vtkmClip(const vtkmClip&) = delete;
  void operator=(const vtkmClip&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmClip_h
