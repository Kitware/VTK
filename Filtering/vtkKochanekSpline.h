/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKochanekSpline.h
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
// .NAME vtkKochanekSpline - computes an interpolating spline using a Kochanek basis.
// .SECTION Description
// Implements the Kochenek interpolating spline described in: Kochanek, D.,
// Bartels, R., "Interpolating Splines with Local Tension, Continuity, and
// Bias Control," Computer Graphics, vol. 18, no. 3, pp. 33-41, July 1984.
// These splines give the user more control over the shape of the curve than
// the cardinal splines implemented in vtkCardinalSpline. Three parameters
// can be specified. All have a range from -1 to 1.
// 
// Tension controls how sharply the curve bends at an input point. A
// value of -1 produces more slack in the curve. A value of 1 tightens
// the curve.
// 
// Continuity controls the continuity of the first derivative at input
// points. 
// 
// Bias controls the direction of the curve at it passes through an input
// point. A value of -1 undershoots the point while a value of 1
// overshoots the point.
// 
// These three parameters give the user broad control over the shape of
// the interpolating spline. The original Kochanek paper describes the
// effects nicely and is recommended reading.

// .SECTION See Also
// vtkSpline vtkCardinalSpline


#ifndef __vtkKochanekSpline_h
#define __vtkKochanekSpline_h

#include <stdio.h>
#include "vtkSpline.h"

class VTK_FILTERING_EXPORT vtkKochanekSpline : public vtkSpline
{
public:
  vtkTypeRevisionMacro(vtkKochanekSpline,vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct a KochanekSpline with the following defaults: DefaultBias = 0,
  // DefaultTension = 0, DefaultContinuity = 0.
  static vtkKochanekSpline *New();

  // Description:
  // Compute Kochanek Spline coefficients.
  void Compute ();
  
  // Description:
  // Evaluate a 1D Kochanek spline.
  float Evaluate (float t);

  // Description:
  // Set the bias for all points. Default is 0.
  vtkSetMacro(DefaultBias,float);
  vtkGetMacro(DefaultBias,float);

  // Description:
  // Set the tension for all points. Default is 0.
  vtkSetMacro(DefaultTension,float);
  vtkGetMacro(DefaultTension,float);

  // Description:
  // Set the continuity for all points. Default is 0.
  vtkSetMacro(DefaultContinuity,float);
  vtkGetMacro(DefaultContinuity,float);

protected:
  vtkKochanekSpline();
  ~vtkKochanekSpline() {};

  void Fit1D (int n, float *x, float *y,
              float tension, float bias, float continuity,
              float coefficients[][4],
              int leftConstraint, float leftValue, int rightConstraint, float rightValue);
  float DefaultBias;
  float DefaultTension;
  float DefaultContinuity;
private:
  vtkKochanekSpline(const vtkKochanekSpline&);  // Not implemented.
  void operator=(const vtkKochanekSpline&);  // Not implemented.
};

#endif

