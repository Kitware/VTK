// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPMultiVariateToRGBA
 * @brief   Extract VOI and/or sub-sample a distributed
 *  structured dataset.
 *
 *
 *  vtkPMultiVariateToRGBA inherits from vtkMultiVariateToRGBA and provides additional
 *  functionality when dealing with a distributed dataset. Specifically, when
 *  sub-sampling a dataset, a gap may be introduced between partitions. This
 *  filter handles such cases correctly by growing the grid to the right to
 *  close the gap.
 *
 * @sa
 *  vtkMultiVariateToRGBA
 */

#ifndef vtkPMultiVariateToRGBA_h
#define vtkPMultiVariateToRGBA_h

#include "vtkFiltersParallelMPIModule.h" // For export macro
#include "vtkMultiVariateToRGBA.h"

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkMPIController;

class VTKFILTERSPARALLELMPI_EXPORT vtkPMultiVariateToRGBA : public vtkMultiVariateToRGBA
{
public:
  static vtkPMultiVariateToRGBA* New();
  vtkTypeMacro(vtkPMultiVariateToRGBA, vtkMultiVariateToRGBA);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPMultiVariateToRGBA();
  ~vtkPMultiVariateToRGBA() override;

  void SetController(vtkMPIController*);
  vtkMPIController* Controller;

private:
  vtkPMultiVariateToRGBA(const vtkPMultiVariateToRGBA&) = delete;
  void operator=(const vtkPMultiVariateToRGBA&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
