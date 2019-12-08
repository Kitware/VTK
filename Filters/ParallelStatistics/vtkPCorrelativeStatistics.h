/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCorrelativeStatistics.h

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
 * @class   vtkPCorrelativeStatistics
 * @brief   A class for parallel bivariate correlative statistics
 *
 * vtkPCorrelativeStatistics is vtkCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
 */

#ifndef vtkPCorrelativeStatistics_h
#define vtkPCorrelativeStatistics_h

#include "vtkCorrelativeStatistics.h"
#include "vtkFiltersParallelStatisticsModule.h" // For export macro

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPCorrelativeStatistics
  : public vtkCorrelativeStatistics
{
public:
  static vtkPCorrelativeStatistics* New();
  vtkTypeMacro(vtkPCorrelativeStatistics, vtkCorrelativeStatistics);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  void Learn(vtkTable* inData, vtkTable* inParameters, vtkMultiBlockDataSet* outMeta) override;

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  void Test(vtkTable*, vtkMultiBlockDataSet*, vtkTable*) override;

protected:
  vtkPCorrelativeStatistics();
  ~vtkPCorrelativeStatistics() override;

  vtkMultiProcessController* Controller;

private:
  vtkPCorrelativeStatistics(const vtkPCorrelativeStatistics&) = delete;
  void operator=(const vtkPCorrelativeStatistics&) = delete;
};

#endif
