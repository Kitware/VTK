/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeDataObjectFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMergeDataObjectFilter - merge dataset and data object field to create dataset with attribute data
// .SECTION Description
// vtkMergeDataObjectFilter is a filter that merges the field from a
// vtkDataObject with a vtkDataSet. The resulting combined dataset can
// then be processed by other filters (e.g.,
// vtkFieldDataToAttributeDataFilter) to create attribute data like
// scalars, vectors, etc.
//
// The filter operates as follows. The field data from the
// vtkDataObject is merged with the input's vtkDataSet and then placed
// in the output. You can choose to place the field data into the cell
// data field, the point data field, or the datasets field (i.e., the
// one inherited from vtkDataSet's superclass vtkDataObject). All this
// data shuffling occurs via reference counting, therefore memory is
// not copied.
//
// One of the uses of this filter is to allow you to read/generate the
// structure of a dataset independent of the attributes. So, for
// example, you could store the dataset geometry/topology in one file,
// and field data in another. Then use this filter in combination with
// vtkFieldDataToAttributeData to create a dataset ready for
// processing in the visualization pipeline.

#ifndef __vtkMergeDataObjectFilter_h
#define __vtkMergeDataObjectFilter_h

#include "vtkDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkMergeDataObjectFilter : public vtkDataSetAlgorithm
{
public:
  static vtkMergeDataObjectFilter *New();
  vtkTypeMacro(vtkMergeDataObjectFilter,vtkDataSetAlgorithm);
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
  void SetOutputFieldToDataObjectField();
  void SetOutputFieldToPointDataField();
  void SetOutputFieldToCellDataField();
  
protected:
  vtkMergeDataObjectFilter();
  ~vtkMergeDataObjectFilter();

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int FillInputPortInformation(int port, vtkInformation *info);

  int OutputField; // which output field

private:
  vtkMergeDataObjectFilter(const vtkMergeDataObjectFilter&);  // Not implemented.
  void operator=(const vtkMergeDataObjectFilter&);  // Not implemented.
};

#endif


