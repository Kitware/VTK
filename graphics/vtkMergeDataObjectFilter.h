/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  vtkMergeDataObjectFilter();
  ~vtkMergeDataObjectFilter();
  static vtkMergeDataObjectFilter *New() {return new vtkMergeDataObjectFilter;};
  const char *GetClassName() {return "vtkMergeDataObjectFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the data, re-executing if required. This method overloads superclasses
  // because there are two inputs: Input & DataObject.
  void Update();

  // Description:
  // Specify the data object to merge with the input dataset.
  vtkSetObjectMacro(DataObject,vtkDataObject);
  vtkGetObjectMacro(DataObject,vtkDataObject);

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
  // Usual data generation method
  void Execute();

  vtkDataObject *DataObject;  // scalars to merge
  int OutputField; // which output field

};

#endif


