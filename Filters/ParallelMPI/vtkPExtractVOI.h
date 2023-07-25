// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPExtractGrid
 * @brief   Extract VOI and/or sub-sample a distributed
 *  structured dataset.
 *
 *
 *  vtkPExtractVOI inherits from vtkExtractVOI and provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkExtractVOI
 */

#ifndef vtkPExtractVOI_h
#define vtkPExtractVOI_h

#include "vtkExtractVOI.h"
#include "vtkFiltersParallelMPIModule.h" // For export macro

// Forward Declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPExtractVOI : public vtkExtractVOI
{
public:
  static vtkPExtractVOI* New();
  vtkTypeMacro(vtkPExtractVOI, vtkExtractVOI);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPExtractVOI();
  ~vtkPExtractVOI() override;

  // Standard VTK Pipeline methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void SetController(vtkMPIController*);
  vtkMPIController* Controller;

private:
  vtkPExtractVOI(const vtkPExtractVOI&) = delete;
  void operator=(const vtkPExtractVOI&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
