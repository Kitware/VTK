/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Cylinder.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkCylinder - implicit function for a cylinder
// .SECTION Description
// vtkCylinder computes the implicit function and function gradient for 
// a cylinder. vtkCylinder is a concrete implementation of vtkImplicitFunction.
// Cylinder is centered at origin and axes of rotation is along z-axis. (Use a
// transform filter if necessary to reposition).
// .SECTION Caveats
// The cylinder is infinite in extent. To truncate the cylinder use the 
// vtkImplicitBoolean in combination with clipping planes.


#ifndef __vtkCylinder_h
#define __vtkCylinder_h

#include "ImpFunc.hh"

class vtkCylinder : public vtkImplicitFunction
{
public:
  vtkCylinder();
  char *GetClassName() {return "vtkCylinder";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Set/Get cylinder radius.
  vtkSetMacro(Radius,float);
  vtkGetMacro(Radius,float);

protected:
  float Radius;

};

#endif


