/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpline.h
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
// .NAME vtkSpline - spline abstract class
// .SECTION Description
// vtkSpline is used to create interpolated data points for specified
// data. vtkSpline is an abstract class: its subclasses vtkCardinalSpline,
// vtkKochenekSpline do the interpolation, The current implementation of
// splines is limited to data dimensions not exceeding four.
// .SECTION See Also
// vtkCardinalSpline vtkKochenekSpline


#ifndef __vtkSpline_h
#define __vtkSpline_h

#include <stdio.h>
#include "vtkPiecewiseFunction.h"

class VTK_EXPORT vtkSpline : public vtkObject
{
public:
  vtkSpline();
  const char *GetClassName() {return "vtkSpline";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get ClampValue. If On, results of the interpolation will be
  // clamped to the min/max of the input data.
  vtkSetMacro(ClampValue,int);
  vtkGetMacro(ClampValue,int);
  vtkBooleanMacro(ClampValue,int);

  // Description:
  // Evaluate a 1D spline.
  float Evaluate (float t);

  // Description:
  // Compute the coefficients for the spline.
  virtual void Compute () = 0;

  // Description:
  // Add a pair of points to be fit with the spline.
  void AddPoint (float t, float x);

  // Description:
  // Remove a point from the data to be fit with the spline.
  void RemovePoint (float t);
 
  // Description:
  // Remove all points from the data.
  void RemoveAllPoints ();

  // Description:
  // Set the type of constraint of the left(right) end points. Three
  // contraints are available:
  // 
  // 1: the first derivative at left(right)most point is set to
  // Left(Right)Value.
  // 
  // 2: the second derivative at left(right)most point is set to
  // Left(Right)Value.
  // 
  // 3: the second derivative at left(right)most points is Left(Right)Value
  // times second derivative at first interior point.
  vtkSetClampMacro(LeftConstraint,int,1,3);
  vtkGetMacro(LeftConstraint,int);
  vtkSetClampMacro(RightConstraint,int,1,3);
  vtkGetMacro(RightConstraint,int);
  vtkSetMacro(LeftValue,float);
  vtkGetMacro(LeftValue,float);
  vtkSetMacro(RightValue,float);
  vtkGetMacro(RightValue,float);

  unsigned long int GetMTime();

protected:
  unsigned long ComputeTime;
  int ClampValue;
  float *Intervals;
  float *Coefficients;
  int LeftConstraint;
  float LeftValue;
  int RightConstraint;
  float RightValue;
  vtkPiecewiseFunction *PiecewiseFunction;
};

#endif

