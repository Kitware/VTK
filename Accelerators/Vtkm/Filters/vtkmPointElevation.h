// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmPointElevation
 * @brief   generate a scalar field along a specified direction
 *
 * vtkmPointElevation is a filter that generates a scalar field along a specified
 * direction. The scalar field values lie within a user specified range, and are
 * generated by computing a projection of each dataset point onto a line. The line
 * can be oriented arbitrarily. A typical example is to generate scalars based
 * on elevation or height above a plane.
 *
 */

#ifndef vtkmPointElevation_h
#define vtkmPointElevation_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // required for correct export
#include "vtkElevationFilter.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmPointElevation : public vtkElevationFilter
{
public:
  vtkTypeMacro(vtkmPointElevation, vtkElevationFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmPointElevation* New();

  ///@{
  /**
   * When this flag is off (the default), then the computation will fall back
   * to the serial VTK version if Viskores fails to run. When the flag is on,
   * the filter will generate an error if Viskores fails to run. This is mostly
   * useful in testing to make sure the expected algorithm is run.
   */
  vtkGetMacro(ForceVTKm, vtkTypeBool);
  vtkSetMacro(ForceVTKm, vtkTypeBool);
  vtkBooleanMacro(ForceVTKm, vtkTypeBool);
  ///@}

protected:
  vtkmPointElevation();
  ~vtkmPointElevation() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ForceVTKm = false;

private:
  vtkmPointElevation(const vtkmPointElevation&) = delete;
  void operator=(const vtkmPointElevation&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmPointElevation_h
