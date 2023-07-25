// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmCleanGrid
 * @brief   removes redundant or unused cells and/or points
 *
 * vtkmCleanGrid is a filter that takes vtkDataSet data as input and
 * generates vtkUnstructuredGrid as output. vtkmCleanGrid will convert all cells
 * to an explicit representation, and if enabled, will remove unused points.
 *
 */

#ifndef vtkmCleanGrid_h
#define vtkmCleanGrid_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkUnstructuredGridAlgorithm.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing vtk-m

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkUnstructuredGrid;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmCleanGrid : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkmCleanGrid, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmCleanGrid* New();

  ///@{
  /**
   * Get/Set if the points from the input that are unused in the output should
   * be removed. This will take extra time but the result dataset may use
   * less memory. Off by default.
   */
  vtkSetMacro(CompactPoints, bool);
  vtkGetMacro(CompactPoints, bool);
  vtkBooleanMacro(CompactPoints, bool);
  ///@}

protected:
  vtkmCleanGrid();
  ~vtkmCleanGrid() override;

  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  bool CompactPoints;

private:
  vtkmCleanGrid(const vtkmCleanGrid&) = delete;
  void operator=(const vtkmCleanGrid&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmCleanGrid_h
