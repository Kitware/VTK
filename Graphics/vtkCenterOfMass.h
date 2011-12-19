/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCenterOfMass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCenterOfMass - Find the center of mass of a set of points.
// .SECTION Description
// vtkCenterOfMass finds the "center of mass" of a vtkPointSet (vtkPolyData
// or vtkUnstructuredGrid). Optionally, the user can specify to use the scalars
// as weights in the computation. If this option, UseScalarsAsWeights, is off,
// each point contributes equally in the calculation.
//
// You must ensure Update() has been called before GetCenter will produce a valid
// value.

#ifndef __vtkCenterOfMass_h
#define __vtkCenterOfMass_h

#include "vtkPointSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkCenterOfMass : public vtkPointSetAlgorithm
{
public:
  static vtkCenterOfMass *New();
  vtkTypeMacro(vtkCenterOfMass,vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of the center of mass computation.
  vtkSetVector3Macro(Center,double);
  vtkGetVector3Macro(Center,double);

  // Description:
  // Set a flag to determine if the points are weighted.
  vtkSetMacro(UseScalarsAsWeights, bool);
  vtkGetMacro(UseScalarsAsWeights, bool);

  // Description:
  // This function is called by RequestData. It exists so that
  // other classes may use this computation without constructing
  // a vtkCenterOfMass object.
  static void ComputeCenterOfMass(vtkPointSet* input, double center[3], bool useWeights);

protected:
  vtkCenterOfMass();

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector );

private:

  vtkCenterOfMass(const vtkCenterOfMass&);  // Not implemented.
  void operator=(const vtkCenterOfMass&);  // Not implemented.

  bool UseScalarsAsWeights;
  double Center[3];
};

#endif
