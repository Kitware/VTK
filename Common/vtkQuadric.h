/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadric.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

#include "vtkImplicitFunction.h"

class VTK_COMMON_EXPORT vtkQuadric : public vtkImplicitFunction
{
public:
  vtkTypeRevisionMacro(vtkQuadric,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description
  // Construct quadric with all coefficients = 1.
  static vtkQuadric *New();

  // Description
  // Evaluate quadric equation.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate the gradient to the quadric equation.
  void EvaluateGradient(float x[3], float g[3]);
  
  // Description
  // Set / get the 10 coefficients of the quadric equation.
  void SetCoefficients(float a[10]);
  void SetCoefficients(float a0, float a1, float a2, float a3, float a4, 
                       float a5, float a6, float a7, float a8, float a9);
  vtkGetVectorMacro(Coefficients,float,10);

protected:
  vtkQuadric();
  ~vtkQuadric() {};

  float Coefficients[10];

private:
  vtkQuadric(const vtkQuadric&);  // Not implemented.
  void operator=(const vtkQuadric&);  // Not implemented.
};

#endif


