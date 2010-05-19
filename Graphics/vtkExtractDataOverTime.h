/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractDataOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractDataOverTime - extract point data from a time sequence for
// a specified point id.
// .SECTION Description
// This filter extracts the point data from a time sequence and specified index
// and creates an output of the same type as the input but with Points 
// containing "number of time steps" points; the point and PointData 
// corresponding to the PointIndex are extracted at each time step and added to
// the output.  A PointData array is added called "Time" (or "TimeData" if
// there is already an array called "Time"), which is the time at each index.

#ifndef __vtkExtractDataOverTime_h
#define __vtkExtractDataOverTime_h

#include "vtkPointSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkExtractDataOverTime : public vtkPointSetAlgorithm
{
public:
  static vtkExtractDataOverTime *New();
  vtkTypeMacro(vtkExtractDataOverTime,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Index of point to extract at each time step
  vtkSetMacro(PointIndex,int);
  vtkGetMacro(PointIndex,int);
  
  // Description:
  // Get the number of time steps
  vtkGetMacro(NumberOfTimeSteps,int);    

protected:
  vtkExtractDataOverTime();
  ~vtkExtractDataOverTime() {};

  int RequestInformation( vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

  int AllocateOutputData(vtkPointSet *input, vtkPointSet *output);

  int            PointIndex;
  int            CurrentTimeIndex;
  int            NumberOfTimeSteps;

private:
  vtkExtractDataOverTime(const vtkExtractDataOverTime&);  // Not implemented.
  void operator=(const vtkExtractDataOverTime&);  // Not implemented.
};

#endif



