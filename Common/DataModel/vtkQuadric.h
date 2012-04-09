/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadric - evaluate implicit quadric function
// .SECTION Description
// vtkQuadric evaluates the quadric function F(x,y,z) = a0*x^2 + a1*y^2 +
// a2*z^2 + a3*x*y + a4*y*z + a5*x*z + a6*x + a7*y + a8*z + a9. vtkQuadric is
// a concrete implementation of vtkImplicitFunction.

#ifndef __vtkQuadric_h
#define __vtkQuadric_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class VTKCOMMONDATAMODEL_EXPORT vtkQuadric : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkQuadric,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct quadric with all coefficients = 1.
  static vtkQuadric *New();

  // Description
  // Evaluate quadric equation.
  double EvaluateFunction(double x[3]);
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate the gradient to the quadric equation.
  void EvaluateGradient(double x[3], double g[3]);

  // Description
  // Set / get the 10 coefficients of the quadric equation.
  void SetCoefficients(double a[10]);
  void SetCoefficients(double a0, double a1, double a2, double a3, double a4,
                       double a5, double a6, double a7, double a8, double a9);
  vtkGetVectorMacro(Coefficients,double,10);

protected:
  vtkQuadric();
  ~vtkQuadric() {};

  double Coefficients[10];

private:
  vtkQuadric(const vtkQuadric&);  // Not implemented.
  void operator=(const vtkQuadric&);  // Not implemented.
};

#endif


