/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitDataSet.h
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
// .NAME vtkImplicitDataSet - treat a dataset as if it were an implicit function
// .SECTION Description
// vtkImplicitDataSet treats any type of dataset as if it were an
// implicit function. This means it computes a function value and 
// gradient. vtkImplicitDataSet is a concrete implementation of 
// vtkImplicitFunction.
//
// vtkImplicitDataSet computes the function (at the point x) by performing 
// cell interpolation. That is, it finds the cell containing x, and then
// uses the cell's interpolation functions to compute an interpolated
// scalar value at x. (A similar approach is used to find the
// gradient, if requested.) Points outside of the dataset are assigned 
// the value of the ivar OutValue, and the gradient value OutGradient.

// .SECTION Caveats
// Any type of dataset can be used as an implicit function as long as it
// has scalar data associated with it.

// .SECTION See Also
// vtkImplicitFunction vtkImplicitVolume vtkClipPolyData vtkCutter
// vtkImplicitWindowFunction

#ifndef __vtkImplicitDataSet_h
#define __vtkImplicitDataSet_h

#include "vtkImplicitFunction.h"
#include "vtkDataSet.h"

class VTK_EXPORT vtkImplicitDataSet : public vtkImplicitFunction
{
public:
  vtkTypeMacro(vtkImplicitDataSet,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  // Construct an vtkImplicitDataSet with no initial dataset; the OutValue
  // set to a large negative number; and the OutGradient set to (0,0,1).
  static vtkImplicitDataSet *New();

  // Description:
  // Return the MTime also considering the DataSet dependency.
  unsigned long GetMTime();

  // Description
  // Evaluate the implicit function. This returns the interpolated scalar value
  // at x[3].
  float EvaluateFunction(float x[3]);
  float EvaluateFunction(float x, float y, float z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;

  // Description
  // Evaluate implicit function gradient.
  void EvaluateGradient(float x[3], float n[3]);

  // Description:
  // Set / get the dataset used for the implicit function evaluation.
  vtkSetObjectMacro(DataSet,vtkDataSet);
  vtkGetObjectMacro(DataSet,vtkDataSet);

  // Description:
  // Set / get the function value to use for points outside of the dataset.
  vtkSetMacro(OutValue,float);
  vtkGetMacro(OutValue,float);

  // Description:
  // Set / get the function gradient to use for points outside of the dataset.
  vtkSetVector3Macro(OutGradient,float);
  vtkGetVector3Macro(OutGradient,float);

protected:
  vtkImplicitDataSet();
  ~vtkImplicitDataSet();
  vtkImplicitDataSet(const vtkImplicitDataSet&);
  void operator=(const vtkImplicitDataSet&);

  vtkDataSet *DataSet;
  float OutValue;
  float OutGradient[3];

  float *Weights; //used to compute interpolation weights
  int Size; //keeps track of length of weights array

};

#endif


