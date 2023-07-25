// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPComputeQuartiles
 * @brief   A class for parallel univariate order statistics
 *
 * `vtkPComputeQuartiles` computes the quartiles of the input table in a distributed
 * environment.
 *
 * @sa vtkPComputeQuartiles
 */

#ifndef vtkPComputeQuartiles_h
#define vtkPComputeQuartiles_h

#include "vtkComputeQuartiles.h"
#include "vtkFiltersParallelStatisticsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOrderStatistics;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPComputeQuartiles : public vtkComputeQuartiles
{
public:
  static vtkPComputeQuartiles* New();
  vtkTypeMacro(vtkPComputeQuartiles, vtkComputeQuartiles);

  ///@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPComputeQuartiles();
  ~vtkPComputeQuartiles() override;

  vtkOrderStatistics* CreateOrderStatisticsFilter() override;

  vtkMultiProcessController* Controller;

private:
  vtkPComputeQuartiles(const vtkPComputeQuartiles&) = delete;
  void operator=(const vtkPComputeQuartiles&) = delete;
};
VTK_ABI_NAMESPACE_END

#endif
