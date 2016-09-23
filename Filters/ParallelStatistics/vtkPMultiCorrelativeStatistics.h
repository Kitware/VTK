/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPMultiCorrelativeStatistics.h

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
 * @class   vtkPMultiCorrelativeStatistics
 * @brief   A class for parallel bivariate correlative statistics
 *
 * vtkPMultiCorrelativeStatistics is vtkMultiCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories for implementing this class.
*/

#ifndef vtkPMultiCorrelativeStatistics_h
#define vtkPMultiCorrelativeStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkMultiCorrelativeStatistics.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPMultiCorrelativeStatistics : public vtkMultiCorrelativeStatistics
{
public:
  static vtkPMultiCorrelativeStatistics* New();
  vtkTypeMacro(vtkPMultiCorrelativeStatistics, vtkMultiCorrelativeStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Performs Reduction
   */
  static void GatherStatistics( vtkMultiProcessController *curController,
                                vtkTable *sparseCov );

protected:
  vtkPMultiCorrelativeStatistics();
  ~vtkPMultiCorrelativeStatistics();

  vtkMultiProcessController* Controller;

  // Execute the parallel calculations required by the Learn option.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

  virtual vtkOrderStatistics* CreateOrderStatisticsInstance();

private:
  vtkPMultiCorrelativeStatistics(const vtkPMultiCorrelativeStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPMultiCorrelativeStatistics&) VTK_DELETE_FUNCTION;
};

#endif
