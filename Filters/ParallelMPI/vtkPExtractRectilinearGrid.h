// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPExtractRectilinearGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  rectilinear grid dataset.
 *
 *
 *  vtkPExtractRectilinearGrid inherits from vtkExtractVOI & provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkExtractRectilinearGrid
 */

#ifndef vtkPExtractRectilinearGrid_h
#define vtkPExtractRectilinearGrid_h

#include "vtkExtractRectilinearGrid.h"
#include "vtkFiltersParallelMPIModule.h" // For export macro

// Forward Declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractRectilinearGrid : public vtkExtractRectilinearGrid
{
public:
  static vtkPExtractRectilinearGrid* New();
  vtkTypeMacro(vtkPExtractRectilinearGrid, vtkExtractRectilinearGrid);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPExtractRectilinearGrid();
  ~vtkPExtractRectilinearGrid() override;

  // Standard VTK Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void SetController(vtkMPIController*);
  vtkMPIController* Controller;

private:
  vtkPExtractRectilinearGrid(const vtkPExtractRectilinearGrid&) = delete;
  void operator=(const vtkPExtractRectilinearGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif /* VTKPEXTRACTRECTILINEARGRID_H_ */
