/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCardinalSpline.h
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

  vtkTypeMacro(vtkCardinalSpline,vtkSpline);
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

