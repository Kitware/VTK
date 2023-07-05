// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCleanUnstructuredGridCells
 * @brief   remove duplicate/degenerate cells
 *
 *
 * Merges degenerate cells. Assumes the input grid does not contain duplicate
 * points. You may want to run vtkCleanUnstructuredGrid first to assert it. If
 * duplicated cells are found they are removed in the output. The filter also
 * handles the case, where a cell may contain degenerate nodes (i.e. one and
 * the same node is referenced by a cell more than once).
 *
 * Programmed 2010 by Dominik Szczerba <dominik@itis.ethz.ch>
 *
 * @sa
 * vtkCleanPolyData
 */

#ifndef vtkCleanUnstructuredGridCells_h
#define vtkCleanUnstructuredGridCells_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkCleanUnstructuredGridCells : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkCleanUnstructuredGridCells* New();

  vtkTypeMacro(vtkCleanUnstructuredGridCells, vtkUnstructuredGridAlgorithm);

  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkCleanUnstructuredGridCells();
  ~vtkCleanUnstructuredGridCells() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkCleanUnstructuredGridCells(const vtkCleanUnstructuredGridCells&) = delete;
  void operator=(const vtkCleanUnstructuredGridCells&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
