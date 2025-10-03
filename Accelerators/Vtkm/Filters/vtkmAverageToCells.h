// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmAverageToPoints
 * @brief   Accelerated point to cell interpolation filter.
 *
 * vtkmAverageToPoints is a filter that transforms point data (i.e., data
 * specified at cell points) into cell data (i.e., data specified per cell).
 * The method of transformation is based on averaging the data
 * values of all points used by particular cell. This filter will also
 * pass through any existing point and cell arrays.
 *
 */

#ifndef vtkmAverageToCells_h
#define vtkmAverageToCells_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkPointDataToCellData.h"
#include "vtkmAlgorithm.h"           // For vtkmAlgorithm
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

#ifndef __VTK_WRAP__
#define vtkPointDataToCellData vtkmAlgorithm<vtkPointDataToCellData>
#endif

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmAverageToCells : public vtkPointDataToCellData
{
public:
  vtkTypeMacro(vtkmAverageToCells, vtkPointDataToCellData);
#ifndef __VTK_WRAP__
#undef vtkPointDataToCellData
#endif
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmAverageToCells* New();

protected:
  vtkmAverageToCells();
  ~vtkmAverageToCells() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmAverageToCells(const vtkmAverageToCells&) = delete;
  void operator=(const vtkmAverageToCells&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmAverageToCells_h
