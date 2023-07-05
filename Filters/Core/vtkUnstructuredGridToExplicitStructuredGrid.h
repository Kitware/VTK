// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUnstructuredGridToExplicitStructuredGrid
 * @brief   Filter which converts an unstructured grid data into an explicit structured grid.
 *          The input grid must have a structured coordinates int cell array.
 *          Moreover, its cell must be listed in the i-j-k order (k varying more often)
 */

#ifndef vtkUnstructuredGridToExplicitStructuredGrid_h
#define vtkUnstructuredGridToExplicitStructuredGrid_h

#include "vtkExplicitStructuredGridAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkUnstructuredGridToExplicitStructuredGrid
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkUnstructuredGridToExplicitStructuredGrid* New();
  vtkTypeMacro(vtkUnstructuredGridToExplicitStructuredGrid, vtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the whole extents for the grid to produce. The size of the grid
   * must match the number of cells in the input.
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  ///@}

protected:
  vtkUnstructuredGridToExplicitStructuredGrid();
  ~vtkUnstructuredGridToExplicitStructuredGrid() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int WholeExtent[6];

private:
  vtkUnstructuredGridToExplicitStructuredGrid(
    const vtkUnstructuredGridToExplicitStructuredGrid&) = delete;
  void operator=(const vtkUnstructuredGridToExplicitStructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
