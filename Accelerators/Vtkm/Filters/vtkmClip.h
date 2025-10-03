// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class vtkmClip
 * @brief Clip a dataset using the accelerated viskores Clip filter.
 *
 * Clip a dataset using either a given value or by using an vtkImplicitFunction
 * Currently the supported implicit functions are Box, Plane, and Sphere.
 *
 */

#ifndef vtkmClip_h
#define vtkmClip_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // For export macro
#include "vtkTableBasedClipDataSet.h"
#include "vtkmAlgorithm.h"           // For vtkmAlgorithm
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

#include <memory> // For std::unique_ptr

#ifndef __VTK_WRAP__
#define vtkTableBasedClipDataSet vtkmAlgorithm<vtkTableBasedClipDataSet>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmClip : public vtkTableBasedClipDataSet
{
public:
  static vtkmClip* New();
  vtkTypeMacro(vtkmClip, vtkTableBasedClipDataSet);
#ifndef __VTK_WRAP__
#undef vtkTableBasedClipDataSet
#endif
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * If true, all input point data arrays will be mapped onto the output
   * dataset. Default is true.
   */
  vtkGetMacro(ComputeScalars, bool);
  vtkSetMacro(ComputeScalars, bool);
  vtkBooleanMacro(ComputeScalars, bool);

protected:
  vtkmClip();
  ~vtkmClip() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool ComputeScalars = true;

  struct internals;

private:
  vtkmClip(const vtkmClip&) = delete;
  void operator=(const vtkmClip&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmClip_h
