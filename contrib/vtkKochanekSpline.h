/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKochanekSpline.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkKochanekSpline - computes an interpolating spline using a Kochanek basis.
// .SECTION Description
// Implements the Kochenek interpolating spline described in: Kochanek,
// D., Bartels, R., "Interpolating Splines with Local Tension,
// Continuity, and Bias Control," Computer Graphics, vol. 18, no. 3,
// pp. 33-41, July 1984.
// These splines give the user more control over the shape of the curve
// than the cardinal splines implemented in vtkCardinalSpline. Three
// parameters can be specified. All have a range from -1 to 1.
// 
// Tension controls how sharply the curve bends at an input point. A
// value of -1 produices more slack in the curve. A value of 1 tightens
// the curve.
// 
// Continuity controls the continuity of the first derivative at input
// points. 
// 
// Bias controls the direction os the curve at it passes through an input
// point. A value of -1 undershoots the point while a value of 1
// overshoots the point.
// 
// These three parameters give the user broad control over the shape of
// the interpolating spline. The original Kochanek paper describes the
// effects nicely and is recommened reading.
// .SECTION See Also
// vtkSpline


#ifndef __vtkKochanekSpline_h
#define __vtkKochanekSpline_h

#include <stdio.h>
#include "vtkSpline.h"

class VTK_EXPORT vtkKochanekSpline : public vtkSpline
{
public:
  vtkKochanekSpline();
  static vtkKochanekSpline *New() {return new vtkKochanekSpline;};
  const char *GetClassName() {return "vtkKochanekSpline";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Compute ();

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
  void Fit1D (int n, float *x, float *y,
              float tension, float bias, float continuity,
              float coefficients[][4],
	      int leftConstraint, float leftValue, int rightConstraint, float rightValue);
  float DefaultBias;
  float DefaultTension;
  float DefaultContinuity;
};

#endif

