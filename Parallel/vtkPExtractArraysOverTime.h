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
// vtkPExtractArraysOverTime

#ifndef __vtkPExtractArraysOverTime_h
#define __vtkPExtractArraysOverTime_h

#include "vtkExtractArraysOverTime.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPExtractArraysOverTime : public vtkExtractArraysOverTime
{
public:
  static vtkPExtractArraysOverTime *New();
  vtkTypeRevisionMacro(vtkPExtractArraysOverTime,vtkExtractArraysOverTime);
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
  void AddRemoteData(vtkRectilinearGrid* routput,
                     vtkRectilinearGrid* output);

  vtkMultiProcessController* Controller;

private:
  vtkPExtractArraysOverTime(const vtkPExtractArraysOverTime&);  // Not implemented.
  void operator=(const vtkPExtractArraysOverTime&);  // Not implemented.
};

#endif



