/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpline.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkSpline - spline abstract class
// .SECTION Description
// vtkSpline is used to create interpolated data points for specified
// data. vtkSpline is an abstract class: its subclasses vtkCardinalSpline
// and vtkKochenekSpline do the interpolation, The current implementation 
// of splines is limited to data dimensions not exceeding four.
//
// Typically a spline is used by adding a sequence of points followed by
// use of an evaluation function (e.g., vtkCardinalSpline::Evaluate()).
// Since these splines are 1D, a point in this context is a independent/
// dependent variable pair. Note that the parameter space of the spline
// ranges from (0,N-1), where N is the number of points in the spline.
//
// Splines can also be set up to be closed or open. Closed splines continue
// from the last point to the first point with continuous function and 
// derivative values. (You don't need to duplicate the first point to close
// the spline, just set ClosedOn.) If the spline is closed, the parameter
// space of the spline becomes (0,N).

// .SECTION See Also
// vtkCardinalSpline vtkKochenekSpline


#ifndef __vtkSpline_h
#define __vtkSpline_h

#include <stdio.h>
#include "vtkObject.h"
#include "vtkPiecewiseFunction.h"

class VTK_FILTERING_EXPORT vtkSpline : public vtkObject
{
public:
  vtkTypeMacro(vtkSpline,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get ClampValue. If On, results of the interpolation will be
  // clamped to the min/max of the input data.
  vtkSetMacro(ClampValue,int);
  vtkGetMacro(ClampValue,int);
  vtkBooleanMacro(ClampValue,int);

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
  // Control whether the spline is open or closed. A closed spline forms
  // a continuous loop: the first and last points are the same, and
  // derivatives are continuous.
  vtkSetMacro(Closed,int);
  vtkGetMacro(Closed,int);
  vtkBooleanMacro(Closed,int);

  // Description:
  // Set the type of constraint of the left(right) end points. Three
  // constraints are available:
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

  // Description:
  // Return the MTime also considering the Piecewise function.
  unsigned long GetMTime();

protected:
  vtkSpline();
  ~vtkSpline ();
  vtkSpline(const vtkSpline&);
  void operator=(const vtkSpline&);

  unsigned long ComputeTime;
  int ClampValue;
  float *Intervals;
  float *Coefficients;
  int LeftConstraint;
  float LeftValue;
  int RightConstraint;
  float RightValue;
  vtkPiecewiseFunction *PiecewiseFunction;
  int Closed;
};

#endif

