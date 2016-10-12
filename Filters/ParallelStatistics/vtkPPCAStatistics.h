/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPPCAStatistics.h

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
 * @class   vtkPPCAStatistics
 * @brief   A class for parallel principal component analysis
 *
 * vtkPPCAStatistics is vtkPCAStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay, David Thompson and Janine Bennett from
 * Sandia National Laboratories for implementing this class.
*/

#ifndef vtkPPCAStatistics_h
#define vtkPPCAStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkPCAStatistics.h"

class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPPCAStatistics : public vtkPCAStatistics
{
public:
  vtkTypeMacro(vtkPPCAStatistics, vtkPCAStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPPCAStatistics* New();

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}


protected:
  vtkPPCAStatistics();
  ~vtkPPCAStatistics();

  vtkMultiProcessController* Controller;

  // Execute the parallel calculations required by the Learn option.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* );

  virtual vtkOrderStatistics* CreateOrderStatisticsInstance();

private:
  vtkPPCAStatistics(const vtkPPCAStatistics&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPPCAStatistics&) VTK_DELETE_FUNCTION;
};

#endif
