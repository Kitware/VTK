/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImpFunc.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkImplicitFunction - abstract interface for implicit functions
// .SECTION Description
// vtkImplicitFunction specifies an abstract interface for implicit 
// functions. Implicit functions are of the form F(x,y,z) = 0. Two primitive 
// operations are required: the ability to evaluate the function and the 
// function gradient at a given point.
//    vtkImplicitFunction provides a mechanism to transform the implicit
// function(s) via a transform filter. This capability can be used to 
// translate, orient, or scale implicit functions. For example, a sphere 
// implicit function can be transformed into an oriented ellipse. This is 
// accomplished by using an instance of vtkTransform.
// .SECTION Caveats
// The transformation matrix transforms a point into the space of the implicit
// function (i.e., the model space). Typically we want to transpose the 
// implicit model into world coordinates. In this case thus inverse of the 
// transform is required.

#ifndef __vtkImplicitFunction_h
#define __vtkImplicitFunction_h

#include "Object.hh"
#include "Trans.hh"

class vtkImplicitFunction : public vtkObject
{
public:
  vtkImplicitFunction();
  ~vtkImplicitFunction() {};
  char *GetClassName() {return "vtkImplicitFunction";};
  void PrintSelf(ostream& os, vtkIndent indent);

  float FunctionValue(float x[3]);
  void FunctionGradient(float x[3], float g[3]);

  // Description:
  // Evaluate function at position x-y-z and return value. Must be implemented
  // by derived class.
  virtual float EvaluateFunction(float x[3]) = 0;

  // Description:
  // Evaluate function gradient at position x-y-z and pass back vector. Must
  // be implemented by derived class.
  virtual void EvaluateGradient(float x[3], float g[3]) = 0;

  // Description:
  // Set/Get transformation matrix to transform implicit function.
  vtkSetObjectMacro(Transform,vtkTransform);
  vtkGetObjectMacro(Transform,vtkTransform);

protected:
  vtkTransform *Transform;

};

#endif
