/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.h
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
// .NAME vtkAppendFilter - appends one or more datasets together into a single unstructured grid
// .SECTION Description
// vtkAppendFilter is a filter that appends one of more datasets into a single
// unstructured grid. All geometry is extracted and appended, but point 
// attributes (i.e., scalars, vectors, normals, field data, etc.) are extracted 
// and appended only if all datasets have the point attributes available. 
// (For example, if one dataset has scalars but another does not, scalars will 
// not be appended.)

// .SECTION See Also
// vtkAppendPolyData

#ifndef __vtkAppendFilter_h
#define __vtkAppendFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"
#include "vtkDataSetCollection.h"

class VTK_GRAPHICS_EXPORT vtkAppendFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkAppendFilter *New();

  vtkTypeMacro(vtkAppendFilter,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to the list of data to append.
  void AddInput(vtkDataSet *in);

  // Description:
  // Get any input of this filter.
  vtkDataSet *GetInput(int idx);
  vtkDataSet *GetInput() 
    {return this->GetInput( 0 );}
  
  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkDataSet *in);

  // Description:
  // Returns a copy of the input array.  Modifications to this list
  // will not be reflected in the actual inputs.
  vtkDataSetCollection *GetInputList();

protected:
  vtkAppendFilter();
  ~vtkAppendFilter();
  vtkAppendFilter(const vtkAppendFilter&);
  void operator=(const vtkAppendFilter&);

  // Usual data generation method
  void Execute();

  // list of data sets to append together.
  // Here as a convenience.  It is a copy of the input array.
  vtkDataSetCollection *InputList;

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkDataSet not a vtkDataObject."); };
  void RemoveInput(vtkDataObject *input)
    { this->vtkProcessObject::RemoveInput(input); };
};


#endif


