/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSimpleInterpolator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSimpleInterpolator - A Simple fast interpolation class
// used for dataset interpolation with vtkTemporalAlgorithm

// .SECTION Description
// This class mimics the key functionality from vtkCardinalspline but
// without using embedded Spline/PiecewiseFunction objects.
// thic class can only be used in a very specific manner, arrays of
// T and data must be prepared and sorted prior to passing into
// the SetArrays function, then the spline may be evaluated for T.
// The class supports Spline and Linear interpolation.

// .SECTION See Also
// vtkTemporalAlgorithm, vtkCardinalSpline

#ifndef __vtkSimpleInterpolator_h
#define __vtkSimpleInterpolator_h

#include "vtkObject.h"

class VTK_HYBRID_EXPORT vtkSimpleInterpolator : public vtkObject
{
public:
  static vtkSimpleInterpolator *New();

  vtkTypeRevisionMacro(vtkSimpleInterpolator, vtkObject);

  // Description:
  // Compute the value at time T using 1D cardinal spline interpolation
  virtual double EvaluateSpline(double t);

  // Description:
  // Compute the value at time T using linear interpolation
  virtual double EvaluateLinear(double t);

  // Description:
  // Set the arrays on which the calculation will be based.
  virtual void SetArrays(int size, double *Tarray, double *Varray, double *Warray, double *coeffs);

  // Description:
  // Set the type of constraint of the left(right) end points. Four
  // constraints are available:
  // 
  // 0: the first derivative at left(right) most point is determined
  // from the line defined from the first(last) two points.
  //
  // 1: the first derivative at left(right) most point is set to
  // Left(Right)Value.
  // 
  // 2: the second derivative at left(right) most point is set to
  // Left(Right)Value.
  // 
  // 3: the second derivative at left(right)most points is Left(Right)Value
  // times second derivative at first interior point.
  vtkSetClampMacro(LeftConstraint,int,0,3);
  vtkGetMacro(LeftConstraint,int);
  vtkSetClampMacro(RightConstraint,int,0,3);
  vtkGetMacro(RightConstraint,int);

  // Description:
  // The values of the derivative on the left and right sides. The value
  // is used only if the left(right) constraint is type 1-3.
  vtkSetMacro(LeftValue,double);
  vtkGetMacro(LeftValue,double);
  vtkSetMacro(RightValue,double);
  vtkGetMacro(RightValue,double);

protected:
   vtkSimpleInterpolator();
  ~vtkSimpleInterpolator() {}

  //
  // Spline specific functions
  //
  void    Fit1DSpline(int size, double *x, double *y, double *w, double coeffs[][4],
            int leftConstraint, double leftValue, int rightConstraint, double rightValue);
  double  ComputeLeftDerivative();
  double  ComputeRightDerivative();
  int     FindIndex(int size, double t);

  //
  // Internal variables
  //
  unsigned long   ComputeTime;
  int             ArraySize;
  double         *Tvalues;
  double         *Dvalues;
  double         *Work;
  double         *Coefficients; 
  int             LeftConstraint;
  double          LeftValue;
  int             RightConstraint;
  double          RightValue;

private:
  vtkSimpleInterpolator(const vtkSimpleInterpolator&);  // Not implemented.
  void operator=(const vtkSimpleInterpolator&);  // Not implemented.
};

#endif

