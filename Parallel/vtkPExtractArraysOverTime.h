/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPExtractArraysOverTime - extract point or cell data over time (parallel)
// .SECTION Description
// vtkPExtractArraysOverTime is a parallelized version of
// vtkExtractArraysOverTime. 
// vtkExtractArraysOverTime extract point or cell data given a selection. For
// every cell or point extracted, vtkExtractArraysOverTime create a vtkTable
// that is placed in an appropriately named block in an output multi-block
// dataset. For global-id based selections or location based selections, it's
// possible that over time the cell/point moves across processes. This filter
// ensures that such extractions spread across processes are combined correctly
// into a single vtkTable.
// This filter produces a valid output on the root node alone, all other nodes,
// simply have empty multi-block dataset with number of blocks matching the root
// (to ensure that all processes have the same structure).
// .SECTION See Also
// vtkExtractArraysOverTime

#ifndef __vtkPExtractArraysOverTime_h
#define __vtkPExtractArraysOverTime_h

#include "vtkExtractArraysOverTime.h"

class vtkMultiProcessController;
class vtkTable;

class VTK_PARALLEL_EXPORT vtkPExtractArraysOverTime : public vtkExtractArraysOverTime
{
public:
  static vtkPExtractArraysOverTime *New();
  vtkTypeMacro(vtkPExtractArraysOverTime,vtkExtractArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

//BTX
  enum Tags
  {
    EXCHANGE_DATA = 1972
  };
//ETX

protected:
  vtkPExtractArraysOverTime();
  ~vtkPExtractArraysOverTime();

  virtual void PostExecute(vtkInformation* request,
                           vtkInformationVector** inputVector,
                           vtkInformationVector* outputVector);
  void AddRemoteData(vtkMultiBlockDataSet* routput,
                     vtkMultiBlockDataSet* output);
  void MergeTables(vtkTable* routput, vtkTable* output);

  vtkMultiProcessController* Controller;

private:
  vtkPExtractArraysOverTime(const vtkPExtractArraysOverTime&);  // Not implemented.
  void operator=(const vtkPExtractArraysOverTime&);  // Not implemented.
};

#endif



