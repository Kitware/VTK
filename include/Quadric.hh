/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Quadric.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkQuadric - evaluate implicit quadric function
// .SECTION Description
// vtkQuadric evaluates the quadric function a0*x^2 + a1*y^2 + a2*z^2 + 
// a3*x*y + a4*y*z + a5*x*z + a6*x + a7*y + a8*z + a9 = 0. vtkQuadric is
// a concrete implementation of vtkImplicitFunction.

#ifndef __vtkQuadric_h
#define __vtkQuadric_h

#include "ImpFunc.hh"

class vtkQuadric : public vtkImplicitFunction
{
public:
  vtkQuadric();
  char *GetClassName() {return "vtkQuadric";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // ImplicitFunction interface
  float EvaluateFunction(float x[3]);
  void EvaluateGradient(float x[3], float g[3]);

  void SetCoefficients(float a[10]);
  void SetCoefficients(float a0, float a1, float a2, float a3, float a4, 
                       float a5, float a6, float a7, float a8, float a9);
  vtkGetVectorMacro(Coefficients,float,10);

protected:
  float Coefficients[10];

};

#endif


