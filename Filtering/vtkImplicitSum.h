/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSum.h
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
// .NAME vtkImplicitSum - implicit sum of other implicit functions
// .SECTION Description
//  vtkImplicitSum produces a linear combination of other implicit functions.
//  The contribution of each function is weighted by a scalar coefficient.
//  The NormalizeByWeight option normalizes the output so that the 
//  scalar weights add up to 1.  Note that this function gives accurate
//  sums and gradients only if the input functions are linear.

#ifndef __vtkImplicitSum_h
#define __vtkImplicitSum_h

#include "vtkImplicitFunctionCollection.h"
#include "vtkFloatArray.h"

#ifndef VTK_FILTERING_EXPORT
#define VTK_FILTERING_EXPORT VTK_EXPORT
#endif

class VTK_FILTERING_EXPORT vtkImplicitSum : public vtkImplicitFunction
{
public:
#ifdef vtkTypeRevisionMacro
  vtkTypeRevisionMacro(vtkImplicitSum,vtkImplicitFunction);
#else
  vtkTypeMacro(vtkImplicitSum,vtkImplicitFunction);
#endif

  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkImplicitSum *New();

  // Description:
  // Evaluate implicit function using current functions and weights.
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description:
  // Evaluate gradient of the weighted sum of functions.  Input functions
  // should be linear.
  void EvaluateGradient(float x[3], float g[3]);

  // Description:
  // Override modified time retrieval because of object dependencies.
  unsigned long GetMTime();

  // Description:
  // Add another implicit function to the list of functions, along with a 
  // weighting factor.
  void AddFunction(vtkImplicitFunction *in, float weight);

  // Description:
  // Add another implicit function to the list of functions, weighting it by
  // a factor of 1.
  void AddFunction(vtkImplicitFunction *in) { this->AddFunction(in, 1.0); }

  // Description:
  // Remove all functions from the list.
  void RemoveAllFunctions();

  // Description:
  // Set the weight (coefficient) of the given function to be weight.
  void SetFunctionWeight(vtkImplicitFunction *f, float weight);

  // Description: 
  // When calculating the function and gradient values of the
  // composite function, setting NormalizeByWeight on will divide the
  // final result by the total weight of the component functions.
  // This process does not otherwise normalize the gradient vector.
  // By default, NormalizeByWeight is off.
  vtkSetMacro(NormalizeByWeight, int);
  vtkGetMacro(NormalizeByWeight, int);
  vtkBooleanMacro(NormalizeByWeight, int);

protected:
  vtkImplicitSum();
  ~vtkImplicitSum();

  vtkImplicitFunctionCollection *FunctionList;
  vtkFloatArray *Weights;
  float TotalWeight;

  void CalculateTotalWeight(void);
  int NormalizeByWeight;

private:
  vtkImplicitSum(const vtkImplicitSum&);  // Not implemented.
  void operator=(const vtkImplicitSum&);  // Not implemented.
};

#endif
