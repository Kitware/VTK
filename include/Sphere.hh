/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Sphere.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkSphere - implicit function for a sphere
// .SECTION Description
// vtkSphere computes the implicit function and/or gradient for a sphere.
// vtkSphere is a concrete implementation of vtkImplicitFunction.

#ifndef __vtkSphere_h
#define __vtkSphere_h

#include "ImpFunc.hh"

class vtkSphere : public vtkImplicitFunction
{
public:
  vtkSphere();
  char *GetClassName() {return "vtkSphere";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float n[3]);

  vtkSetMacro(Radius,float);
  vtkGetMacro(Radius,float);

  vtkSetVector3Macro(Center,float);
  vtkGetVectorMacro(Center,float,3);

protected:
  float Radius;
  float Center[3];

};

#endif


