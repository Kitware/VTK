/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPolyData.h
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
// .NAME vtkAppendPolyData - appends one or more polygonal datasets together
// .SECTION Description
// 
// vtkAppendPolyData is a filter that appends one of more polygonal datasets
// into a single polygonal dataset. All geometry is extracted and appended,
// but point and cell attributes (i.e., scalars, vectors, normals) are
// extracted and appended only if all datasets have the point and/or cell
// attributes available.  (For example, if one dataset has point scalars but
// another does not, point scalars will not be appended.)

// .SECTION See Also
// vtkAppendFilter

#ifndef __vtkAppendPolyData_h
#define __vtkAppendPolyData_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkAppendPolyData : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkAppendPolyData *New();

  vtkTypeMacro(vtkAppendPolyData,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // UserManagedInputs allows the user to set inputs by number instead of
  // using the AddInput/RemoveInput functions. Calls to
  // SetNumberOfInputs/SetInputByNumber should not be mixed with calls
  // to AddInput/RemoveInput. By default, UserManagedInputs is false.
  vtkSetMacro(UserManagedInputs,int);
  vtkGetMacro(UserManagedInputs,int);
  vtkBooleanMacro(UserManagedInputs,int);

  // Description:
  // Add a dataset to the list of data to append. Should not be
  // used when UserManagedInputs is true, use SetInputByNumber instead.
  void AddInput(vtkPolyData *);

  // Description:
  // Remove a dataset from the list of data to append. Should not be
  // used when UserManagedInputs is true, use SetInputByNumber (NULL) instead.
  void RemoveInput(vtkPolyData *);

  // Description:
  // Get any input of this filter.
  vtkPolyData *GetInput(int idx);
  vtkPolyData *GetInput() { return this->GetInput( 0 ); };

  // Description:
  // Directly set(allocate) number of inputs, should only be used
  // when UserManagedInputs is true.
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputByNumber(int num, vtkPolyData *input);

  // Description:
  // ParallelStreaming is for a particular application.
  // It causes this filter to ask for a different piece
  // from each of its inputs.  If all the inputs are the same,
  // then the output of this append filter is the whole dataset
  // pieced back together.  Duplicate points are create 
  // along the seams.  The purpose of this feature is to get 
  // data parallelism at a course scale.  Each of the inputs
  // can be generated in a different process at the same time.
  vtkSetMacro(ParallelStreaming, int); 
  vtkGetMacro(ParallelStreaming, int); 
  vtkBooleanMacro(ParallelStreaming, int); 

protected:
  vtkAppendPolyData();
  ~vtkAppendPolyData();
  vtkAppendPolyData(const vtkAppendPolyData&) {};
  void operator=(const vtkAppendPolyData&) {};

  // Flag for selecting parallel streaming behavior
  int ParallelStreaming;

  // Usual data generation method
  void Execute();

  void ComputeInputUpdateExtents(vtkDataObject *output);

  // An efficient way to append data/cells.
  void AppendData(vtkDataArray *dest, vtkDataArray *src, int offset);
  void AppendDifferentPoints(vtkDataArray *dest, vtkDataArray *src,
                             int offset);
  vtkIdType *AppendCells(vtkIdType *pDest, vtkCellArray *src, int offset);

 private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkPolyData not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input);
    this->vtkProcessObject::SqueezeInputArray();};
  int UserManagedInputs;
};

#endif


