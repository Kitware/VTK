/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.h
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
// .NAME vtkMergeDataObjectFilter - merge dataset and data object field to create dataset with attribute data
// .SECTION Description
// vtkMergeDataObjectFilter is a filter that merges the field from a 
// vtkDataObject with a vtkDataSet. The resulting combined dataset can then
// be processed by other filters (e.g., vtkFieldDataToAttributeDataFilter)
// to create attribute data like scalars, vectors, etc.
//
// The filter operates as follows. The field data from the vtkDataObject is
// merged with the input's vtkDataSet and then placed in the output. You can
// choose to place the field data into the cell data field, the point data field,
// or the datasets field (i.e., the one inherited from vtkDataSet's
// superclass vtkDataObject). All this data shuffling occurs via reference
// counting, therefore memory is not copied.
//
// One of the uses of this filter is to allow you to read/generate the
// structure of a dataset independent of the attributes. So, for example, you
// could store the dataset geometry/topology in one file, and field data in
// another. Then use this filter in combination with
// vtkFieldDataToAttributeData to create a dataset ready for processing in
// the visualization pipeline.

#ifndef __vtkMergeDataObjectFilter_h
#define __vtkMergeDataObjectFilter_h

#include "vtkDataSetToDataSetFilter.h"
#include "vtkFieldDataToAttributeDataFilter.h"

class VTK_EXPORT vtkMergeDataObjectFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkMergeDataObjectFilter *New();
  vtkTypeMacro(vtkMergeDataObjectFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the data object to merge with the input dataset.
  void SetDataObject(vtkDataObject *object);
  vtkDataObject *GetDataObject();

  // Description:
  // Specify where to place the field data during the merge process.  There
  // are three choices: the field data associated with the vtkDataObject
  // superclass; the point field attribute data; and the cell field attribute
  // data.
  vtkSetMacro(OutputField,int);
  vtkGetMacro(OutputField,int);
  void SetOutputFieldToDataObjectField() 
    {this->SetOutputField(VTK_DATA_OBJECT_FIELD);};
  void SetOutputFieldToPointDataField() 
    {this->SetOutputField(VTK_POINT_DATA_FIELD);};
  void SetOutputFieldToCellDataField() 
    {this->SetOutputField(VTK_CELL_DATA_FIELD);};
  
protected:
  vtkMergeDataObjectFilter();
  ~vtkMergeDataObjectFilter();
  vtkMergeDataObjectFilter(const vtkMergeDataObjectFilter&);
  void operator=(const vtkMergeDataObjectFilter&);

  // Usual data generation method
  void Execute();

  int OutputField; // which output field

};

#endif


