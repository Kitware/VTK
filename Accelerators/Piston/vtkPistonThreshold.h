/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonThreshold -  A filter that contours on the GPU
// .SECTION Description
// This filter uses LANL's Piston library to isocontour on the GPU.

#ifndef __vtkPistonThreshold_h
#define __vtkPistonThreshold_h

#include "vtkPistonAlgorithm.h"

class VTKACCELERATORSPISTON_EXPORT vtkPistonThreshold : public vtkPistonAlgorithm
{
public:
  vtkTypeMacro(vtkPistonThreshold,vtkPistonAlgorithm);
  static vtkPistonThreshold *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  //Description:
  //Choose the lower value of the threshold.
  vtkSetMacro(MinValue, float);
  vtkGetMacro(MinValue, float);

  //Description:
  //Choose the upper value of the threshold.
  vtkSetMacro(MaxValue, float);
  vtkGetMacro(MaxValue, float);
protected:
  vtkPistonThreshold();
  ~vtkPistonThreshold();

  // Description:
  // Method that does the actual calculation.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);


  float MinValue;
  float MaxValue;

private:
  vtkPistonThreshold(const vtkPistonThreshold&);  // Not implemented.
  void operator=(const vtkPistonThreshold&);  // Not implemented.

};

#endif
