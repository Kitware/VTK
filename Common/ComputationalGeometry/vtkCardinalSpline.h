/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCardinalSpline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCardinalSpline - computes an interpolating spline using a
// a Cardinal basis.

// .SECTION Description
// vtkCardinalSpline is a concrete implementation of vtkSpline using a
// Cardinal basis.

// .SECTION See Also
// vtkSpline vtkKochanekSpline


#ifndef vtkCardinalSpline_h
#define vtkCardinalSpline_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkSpline.h"

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkCardinalSpline : public vtkSpline
{
public:
  static vtkCardinalSpline *New();

  vtkTypeMacro(vtkCardinalSpline,vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Compute Cardinal Splines for each dependent variable
  void Compute ();

  // Description:
  // Evaluate a 1D cardinal spline.
  virtual double Evaluate (double t);

  // Description:
  // Deep copy of cardinal spline data.
  virtual void DeepCopy(vtkSpline *s);

protected:
  vtkCardinalSpline();
  ~vtkCardinalSpline() {}

  void Fit1D (int n, double *x, double *y, double *w, double coefficients[][4],
              int leftConstraint, double leftValue, int rightConstraint,
              double rightValue);

  void FitClosed1D (int n, double *x, double *y, double *w,
                    double coefficients[][4]);

private:
  vtkCardinalSpline(const vtkCardinalSpline&);  // Not implemented.
  void operator=(const vtkCardinalSpline&);  // Not implemented.
};

#endif

