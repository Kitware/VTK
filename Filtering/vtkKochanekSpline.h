/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKochanekSpline.h
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
  vtkTypeMacro(vtkKochanekSpline,vtkSpline);
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

