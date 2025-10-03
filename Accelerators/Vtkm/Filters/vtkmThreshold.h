// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmThreshold
 * @brief   extracts cells where scalar value in cell satisfies threshold criterion
 *
 * vtkmThreshold is a filter that extracts cells from any dataset type that
 * satisfy a threshold criterion. A cell satisfies the criterion if the
 * scalar value of every point or cell satisfies the criterion. The
 * criterion takes the form of between two values. The output of this
 * filter is an unstructured grid.
 *
 * Note that scalar values are available from the point and cell attribute
 * data. By default, point data is used to obtain scalars, but you can
 * control this behavior. See the AttributeMode ivar below.
 *
 */
#ifndef vtkmThreshold_h
#define vtkmThreshold_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkThreshold.h"
#include "vtkmAlgorithm.h"           // For vtkmAlgorithm
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

#ifndef __VTK_WRAP__
#define vtkThreshold vtkmAlgorithm<vtkThreshold>
#endif

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmThreshold : public vtkThreshold
{
public:
  vtkTypeMacro(vtkmThreshold, vtkThreshold);
#ifndef __VTK_WRAP__
#undef vtkThreshold
#endif
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmThreshold* New();

protected:
  vtkmThreshold();
  ~vtkmThreshold() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmThreshold(const vtkmThreshold&) = delete;
  void operator=(const vtkmThreshold&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmThreshold_h
