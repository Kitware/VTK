/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonContour.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonContour -  A filter that contours on the GPU
// .SECTION Description
// This filter uses LANL's Piston library to isocontour on the GPU.

#ifndef vtkPistonContour_h
#define vtkPistonContour_h

#include "vtkPistonAlgorithm.h"

class VTKACCELERATORSPISTON_EXPORT vtkPistonContour : public vtkPistonAlgorithm
{
public:
  vtkTypeMacro(vtkPistonContour,vtkPistonAlgorithm);
  static vtkPistonContour *New();
  void PrintSelf(ostream &os, vtkIndent indent);

  //Description:
  //Choose the isovalue to contour on.
  vtkSetMacro(IsoValue, float);
  vtkGetMacro(IsoValue, float);
protected:
  vtkPistonContour();
  ~vtkPistonContour();

  // Description:
  // Method that does the actual calculation. Funnels down to ExecuteData.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  float IsoValue;

private:
  vtkPistonContour(const vtkPistonContour&);  // Not implemented.
  void operator=(const vtkPistonContour&);  // Not implemented.

};

#endif
