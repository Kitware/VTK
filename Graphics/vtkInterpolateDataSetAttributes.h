/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolateDataSetAttributes.h
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
// .NAME vtkInterpolateDataSetAttributes - interpolate scalars, vectors, etc. and other dataset attributes
// .SECTION Description
// vtkInterpolateDataSetAttributes is a filter that interpolates data set
// attribute values between input data sets. The input to the filter
// must be datasets of the same type, same number of cells, and same 
// number of points. The output of the filter is a data set of the same
// type as the input dataset and whose attribute values have been 
// interpolated at the parametric value specified.
//
// The filter is used by specifying two or more input data sets (total of N),
// and a parametric value t (0 <= t <= N-1). The output will contain
// interpolated data set attributes common to all input data sets. (For
// example, if one input has scalars and vectors, and another has just
// scalars, then only scalars will be interpolated and output.)

#ifndef __vtkInterpolateDataSetAttributes_h
#define __vtkInterpolateDataSetAttributes_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkDataSetCollection.h"

class VTK_EXPORT vtkInterpolateDataSetAttributes : public vtkDataSetToDataSetFilter
{
public:
  static vtkInterpolateDataSetAttributes *New();
  vtkTypeMacro(vtkInterpolateDataSetAttributes,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to interpolate.
  void AddInput(vtkDataSet *in);

  // Description:
  // Return the list of inputs to this filter.
  vtkDataSetCollection *GetInputList();
  
  // Description:
  // Specify interpolation parameter t.
  vtkSetClampMacro(T,float,0.0,VTK_LARGE_FLOAT);
  vtkGetMacro(T,float);

protected:
  vtkInterpolateDataSetAttributes();
  ~vtkInterpolateDataSetAttributes();
  vtkInterpolateDataSetAttributes(const vtkInterpolateDataSetAttributes&);
  void operator=(const vtkInterpolateDataSetAttributes&);

  void Execute();
  
  vtkDataSetCollection *InputList; // list of data sets to interpolate 
  float T; // interpolation parameter

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
};

#endif


