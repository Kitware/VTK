/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDescriptiveStatistics.h

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
// .NAME vtkPDescriptiveStatistics - A class for parallel univariate descriptive statistics
// .SECTION Description
// vtkPDescriptiveStatistics is vtkDescriptiveStatistics subclass for parallel datasets.
// It learns and derives the global statistical model on each node, but assesses each
// individual data points on the node that owns it.

// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.

#ifndef __vtkPDescriptiveStatistics_h
#define __vtkPDescriptiveStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkDescriptiveStatistics.h"

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPDescriptiveStatistics : public vtkDescriptiveStatistics
{
public:
  static vtkPDescriptiveStatistics* New();
  vtkTypeMacro(vtkPDescriptiveStatistics, vtkDescriptiveStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the multiprocess controller. If no controller is set,
  // single process is assumed.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Execute the parallel calculations required by the Learn option.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );

protected:
  vtkPDescriptiveStatistics();
  ~vtkPDescriptiveStatistics();

  vtkMultiProcessController* Controller;
private:
  vtkPDescriptiveStatistics(const vtkPDescriptiveStatistics&); // Not implemented.
  void operator=(const vtkPDescriptiveStatistics&); // Not implemented.
};

#endif
