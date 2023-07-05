// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPComputeQuantiles
 * @brief   A class for parallel univariate order statistics
 *
 * `vtkPComputeQuantiles` computes the quantiles of the input table in a distributed
 * environment.
 *
 * @sa vtkPComputeQuantiles
 */

#ifndef vtkPComputeQuantiles_h
#define vtkPComputeQuantiles_h

#include "vtkComputeQuantiles.h"
#include "vtkFiltersParallelStatisticsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOrderStatistics;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPComputeQuantiles : public vtkComputeQuantiles
{
public:
  static vtkPComputeQuantiles* New();
  vtkTypeMacro(vtkPComputeQuantiles, vtkComputeQuantiles);

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPComputeQuantiles();
  ~vtkPComputeQuantiles() override;

  vtkOrderStatistics* CreateOrderStatisticsFilter() override;

  vtkMultiProcessController* Controller = nullptr;

private:
  vtkPComputeQuantiles(const vtkPComputeQuantiles&) = delete;
  void operator=(const vtkPComputeQuantiles&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
