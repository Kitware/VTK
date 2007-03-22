/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractArraysOverTime -
// .SECTION Description

#ifndef __vtkExtractArraysOverTime_h
#define __vtkExtractArraysOverTime_h

#include "vtkRectilinearGridAlgorithm.h"

class vtkDataSet;
class vtkRectilinearGrid;

class VTK_GRAPHICS_EXPORT vtkExtractArraysOverTime : public vtkRectilinearGridAlgorithm
{
public:
  static vtkExtractArraysOverTime *New();
  vtkTypeRevisionMacro(vtkExtractArraysOverTime,vtkRectilinearGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the number of time steps
  vtkGetMacro(NumberOfTimeSteps,int);    

protected:
  vtkExtractArraysOverTime();
  ~vtkExtractArraysOverTime() {};

  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector, 
                         vtkInformationVector* outputVector);

  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

  int AllocateOutputData(vtkDataSet *input, vtkRectilinearGrid *output);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  int CurrentTimeIndex;
  int NumberOfTimeSteps;

private:
  vtkExtractArraysOverTime(const vtkExtractArraysOverTime&);  // Not implemented.
  void operator=(const vtkExtractArraysOverTime&);  // Not implemented.
};

#endif



