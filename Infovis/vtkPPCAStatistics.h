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
// .NAME vtkPPCAStatistics - A class for parallel principal component analysis
// .SECTION Description
// vtkPPCAStatistics is vtkPCAStatistics subclass for parallel datasets.
// It learns and derives the global statistical model on each node, but assesses each 
// individual data points on the node that owns it.

// .SECTION Thanks
// Thanks to Philippe Pebay, David Thompson and Janine Bennett from 
// Sandia National Laboratories for implementing this class.

#ifndef __vtkPPCAStatistics_h
#define __vtkPPCAStatistics_h

#include "vtkPCAStatistics.h"

class vtkMultiProcessController;

class VTK_INFOVIS_EXPORT vtkPPCAStatistics : public vtkPCAStatistics
{
public:
  vtkTypeMacro(vtkPPCAStatistics, vtkPCAStatistics);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPPCAStatistics* New();

  // Description:
  // Get/Set the multiprocess controller. If no controller is set,
  // single process is assumed.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:

protected:
  vtkPPCAStatistics();
  ~vtkPPCAStatistics();

  vtkMultiProcessController* Controller;

  // Execute the parallel calculations required by the Learn option.
  virtual void Learn( vtkTable* inData,
                      vtkTable* inParameters,
                      vtkMultiBlockDataSet* outMeta );
private:
  vtkPPCAStatistics(const vtkPPCAStatistics&); // Not implemented.
  void operator=(const vtkPPCAStatistics&); // Not implemented.
};

#endif

