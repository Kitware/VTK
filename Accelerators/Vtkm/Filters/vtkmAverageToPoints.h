// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmAverageToPoints
 * @brief   Accelerated cell to point interpolation filter.
 *
 * vtkmAverageToPoints is a filter that transforms cell data (i.e., data
 * specified per cell) into point data (i.e., data specified at cell
 * points). The method of transformation is based on averaging the data
 * values of all cells using a particular point. This filter will also
 * pass through any existing point and cell arrays.
 *
 */
#ifndef vtkmAverageToPoints_h
#define vtkmAverageToPoints_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkCellDataToPointData.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmAverageToPoints : public vtkCellDataToPointData
{
public:
  vtkTypeMacro(vtkmAverageToPoints, vtkCellDataToPointData);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmAverageToPoints* New();

protected:
  vtkmAverageToPoints();
  ~vtkmAverageToPoints() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmAverageToPoints(const vtkmAverageToPoints&) = delete;
  void operator=(const vtkmAverageToPoints&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmAverageToPoints_h
