/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadric.h
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
// .NAME vtkQuadric - evaluate implicit quadric function
// .SECTION Description
// vtkQuadric evaluates the quadric function F(x,y,z) = a0*x^2 + a1*y^2 + 
// a2*z^2 + a3*x*y + a4*y*z + a5*x*z + a6*x + a7*y + a8*z + a9. vtkQuadric is
// a concrete implementation of vtkImplicitFunction.

#ifndef __vtkQuadric_h
#define __vtkQuadric_h

#include "vtkImplicitFunction.h"

class VTK_COMMON_EXPORT vtkQuadric : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkQuadric,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description
  // Construct quadric with all coefficients = 1.
  static vtkQuadric *New();

  // Description
  // Evaluate quadric equation.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate the gradient to the quadric equation.
  void EvaluateGradient(float x[3], float g[3]);
  
  // Description
  // Set / get the 10 coefficients of the quadric equation.
  void SetCoefficients(float a[10]);
  void SetCoefficients(float a0, float a1, float a2, float a3, float a4, 
                       float a5, float a6, float a7, float a8, float a9);
  vtkGetVectorMacro(Coefficients,float,10);

protected:
  vtkQuadric();
  ~vtkQuadric() {};
  vtkQuadric(const vtkQuadric&);
  void operator=(const vtkQuadric&);

  float Coefficients[10];

};

#endif


