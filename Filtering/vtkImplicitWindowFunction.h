/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitWindowFunction.h
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
// .NAME vtkImplicitWindowFunction - implicit function maps another implicit function to lie within a specified range
// .SECTION Description
// vtkImplicitWindowFunction is used to modify the output of another
// implicit function to lie within a specified "window", or function
// range. This can be used to add "thickness" to cutting or clipping
// functions. 
//
// This class works as follows. First, it evaluates the function value of the 
// user-specified implicit function. Then, based on the window range specified,
// it maps the function value into the window values specified. 
//

// .SECTION See Also
// vtkImplicitFunction

#ifndef __vtkImplicitWindowFunction_h
#define __vtkImplicitWindowFunction_h

#include <math.h>
#include "vtkImplicitFunction.h"

class VTK_EXPORT vtkImplicitWindowFunction : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitWindowFunction,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with window range (0,1) and window values (0,1).
  static vtkImplicitWindowFunction *New();

  // Description
  // Evaluate window function.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate window function gradient. Just return implicit function gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Specify an implicit function to operate on.
  vtkSetObjectMacro(ImplicitFunction,vtkImplicitFunction);
  vtkGetObjectMacro(ImplicitFunction,vtkImplicitFunction);

  // Description:
  // Specify the range of function values which are considered to lie within
  // the window. WindowRange[0] is assumed to be less than WindowRange[1].
  vtkSetVector2Macro(WindowRange,float);
  vtkGetVectorMacro(WindowRange,float,2);

  // Description:
  // Specify the range of output values that the window range is mapped
  // into. This is effectively a scaling and shifting of the original
  // function values.
  vtkSetVector2Macro(WindowValues,float);
  vtkGetVectorMacro(WindowValues,float,2);

  // Description:
  // Override modified time retrieval because of object dependencies.
  unsigned long GetMTime();

protected:
  vtkImplicitWindowFunction();
  ~vtkImplicitWindowFunction();
  vtkImplicitWindowFunction(const vtkImplicitWindowFunction&);
  void operator=(const vtkImplicitWindowFunction&);

  vtkImplicitFunction *ImplicitFunction;
  float WindowRange[2];
  float WindowValues[2];

};

#endif


