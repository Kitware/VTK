/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeFilter.h
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
// .NAME vtkMergeFilter - extract separate components of data from different datasets
// .SECTION Description
// vtkMergeFilter is a filter that extracts separate components of data from
// different datasets and merges them into a single dataset. The output from
// this filter is of the same type as the input (i.e., vtkDataSet.) It treats 
// both cell and point data set attributes.

#ifndef __vtkMergeFilter_h
#define __vtkMergeFilter_h

#include "vtkDataSetToDataSetFilter.h"

class vtkFieldList;

class VTK_GRAPHICS_EXPORT vtkMergeFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkMergeFilter *New();
  vtkTypeMacro(vtkMergeFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify object from which to extract geometry information.
  void SetGeometry(vtkDataSet *input) {this->SetInput(input);};
  vtkDataSet *GetGeometry() {return this->GetInput();};

  // Description:
  // Specify object from which to extract scalar information.
  void SetScalars(vtkDataSet *);
  vtkDataSet *GetScalars();

  // Description:
  // Set / get the object from which to extract vector information.
  void SetVectors(vtkDataSet *);
  vtkDataSet *GetVectors();
  
  // Description:
  // Set / get the object from which to extract normal information.
  void SetNormals(vtkDataSet *);
  vtkDataSet *GetNormals();
  
  // Description:
  // Set / get the object from which to extract texture coordinates
  // information.
  void SetTCoords(vtkDataSet *);
  vtkDataSet *GetTCoords();

  // Description:
  // Set / get the object from which to extract tensor data.
  void SetTensors(vtkDataSet *);
  vtkDataSet *GetTensors();

  // Description:
  // Set the object from which to extract a field and the name
  // of the field
  void AddField(const char* name, vtkDataSet* input);

protected:
  vtkMergeFilter();
  ~vtkMergeFilter();

  // Usual data generation method
  void Execute();
  void ComputeInputUpdateExtents(vtkDataObject *data);

  vtkFieldList* FieldList;
private:
  vtkMergeFilter(const vtkMergeFilter&);  // Not implemented.
  void operator=(const vtkMergeFilter&);  // Not implemented.
  };

#endif


