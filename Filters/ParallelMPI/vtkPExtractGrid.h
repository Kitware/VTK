// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPExtractGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  structured dataset.
 *
 *
 *  vtkPExtractGrid inherits from vtkExtractGrid and provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkExtractGrid
 */

#ifndef vtkPExtractGrid_h
#define vtkPExtractGrid_h

#include "vtkExtractGrid.h"
#include "vtkFiltersParallelMPIModule.h" // For export macro

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractGrid : public vtkExtractGrid
{
public:
  static vtkPExtractGrid* New();
  vtkTypeMacro(vtkPExtractGrid, vtkExtractGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPExtractGrid();
  ~vtkPExtractGrid() override;

  // Standard VTK Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void SetController(vtkMPIController*);
  vtkMPIController* Controller;

private:
  vtkPExtractGrid(const vtkPExtractGrid&) = delete;
  void operator=(const vtkPExtractGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
