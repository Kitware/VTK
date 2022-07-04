/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPComputeQuantiles.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
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

#endif
