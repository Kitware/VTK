/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCardinalSpline.h
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
// .NAME vtkCardinalSpline - computes an interpolating spline using a
// a Cardinal basis.

// .SECTION Description
// vtkCardinalSpline is a concrete implementation of vtkSpline using a
// Cardinal basis.

// .SECTION See Also
// vtkSpline vtkKochanekSpline


#ifndef __vtkCardinalSpline_h
#define __vtkCardinalSpline_h

#include "vtkSpline.h"

class VTK_FILTERING_EXPORT vtkCardinalSpline : public vtkSpline
{
public:
  static vtkCardinalSpline *New();

  vtkTypeRevisionMacro(vtkCardinalSpline,vtkSpline);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Compute Cardinal Splines for each dependent variable
  void Compute ();

  // Description:
  // Evaluate a 1D cardinal spline.
  float Evaluate (float t);

protected:
  vtkCardinalSpline();
  ~vtkCardinalSpline() {};

  void Fit1D (int n, float *x, float *y, float *w, float coefficients[][4],
              int leftConstraint, float leftValue, int rightConstraint, float rightValue);
  void FitClosed1D (int n, float *x, float *y, float *w, 
                    float coefficients[][4]);
private:
  vtkCardinalSpline(const vtkCardinalSpline&);  // Not implemented.
  void operator=(const vtkCardinalSpline&);  // Not implemented.
};

#endif

