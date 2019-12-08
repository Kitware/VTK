/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridToExplicitStructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridToExplicitStructuredGrid
 * @brief   Filter which converts an unstructured grid data into an explicit structured grid.
 *          The input grid must have a structured coordinates int cell array.
 *          Moreover, its cell must be listed in the i-j-k order (k varying more ofter)
 */

#ifndef vtkUnstructuredGridToExplicitStructuredGrid_h
#define vtkUnstructuredGridToExplicitStructuredGrid_h

#include "vtkExplicitStructuredGridAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

class VTKFILTERSCORE_EXPORT vtkUnstructuredGridToExplicitStructuredGrid
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkUnstructuredGridToExplicitStructuredGrid* New();
  vtkTypeMacro(vtkUnstructuredGridToExplicitStructuredGrid, vtkExplicitStructuredGridAlgorithm);

  //@{
  /**
   * Get/Set the whole extents for the grid to produce. The size of the grid
   * must match the number of cells in the input.
   */
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);
  //@}

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

#endif
