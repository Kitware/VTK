/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPComputeQuartiles.h

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

#endif
