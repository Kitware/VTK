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
// .NAME vtkPCorrelativeStatistics - A class for parallel bivariate correlative statistics
// .SECTION Description
// vtkPCorrelativeStatistics is vtkCorrelativeStatistics subclass for parallel datasets.
// It learns and derives the global statistical model on each node, but assesses each 
// individual data points on the node that owns it.

// .SECTION Thanks
// Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.

#ifndef __vtkPCorrelativeStatistics_h
#define __vtkPCorrelativeStatistics_h

#include "vtkCorrelativeStatistics.h"

class vtkMultiBlockDataSet;
class vtkMultiProcessController;

class VTK_INFOVIS_EXPORT vtkPCorrelativeStatistics : public vtkCorrelativeStatistics
{
public:
  static vtkPCorrelativeStatistics* New();
  vtkTypeMacro(vtkPCorrelativeStatistics, vtkCorrelativeStatistics);
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
  vtkPCorrelativeStatistics();
  ~vtkPCorrelativeStatistics();

  vtkMultiProcessController* Controller;
private:
  vtkPCorrelativeStatistics(const vtkPCorrelativeStatistics&); // Not implemented.
  void operator=(const vtkPCorrelativeStatistics&); // Not implemented.
};

#endif

