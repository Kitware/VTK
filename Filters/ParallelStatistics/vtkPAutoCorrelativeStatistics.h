/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPAutoCorrelativeStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPAutoCorrelativeStatistics - A class for parallel auto-correlative statistics
// .SECTION Description
// vtkPAutoCorrelativeStatistics is vtkAutoCorrelativeStatistics subclass for parallel datasets.
// It learns and derives the global statistical model on each node, but assesses each
// individual data points on the node that owns it.

// .SECTION Thanks
// This class was written by Philippe Pebay, Kitware SAS 2012.

#ifndef __vtkPAutoCorrelativeStatistics_h
#define __vtkPAutoCorrelativeStatistics_h

#include "vtkFiltersParallelStatisticsModule.h" // For export macro
#include "vtkAutoCorrelativeStatistics.h"

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPAutoCorrelativeStatistics : public vtkAutoCorrelativeStatistics
{
public:
  static vtkPAutoCorrelativeStatistics* New();
  vtkTypeMacro(vtkPAutoCorrelativeStatistics, vtkAutoCorrelativeStatistics);
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

  // Description:
  // Execute the calculations required by the Test option.
  // NB: Not implemented for more than 1 processor
  virtual void Test( vtkTable*,
                     vtkMultiBlockDataSet*,
                     vtkTable* );

protected:
  vtkPAutoCorrelativeStatistics();
  ~vtkPAutoCorrelativeStatistics();

  vtkMultiProcessController* Controller;
private:
  vtkPAutoCorrelativeStatistics(const vtkPAutoCorrelativeStatistics&); // Not implemented.
  void operator=(const vtkPAutoCorrelativeStatistics&); // Not implemented.
};

#endif
