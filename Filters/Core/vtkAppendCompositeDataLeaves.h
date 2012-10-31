/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendCompositeDataLeaves.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAppendCompositeDataLeaves - appends one or more composite datasets with the same structure together into a single output composite dataset
// .SECTION Description
// vtkAppendCompositeDataLeaves is a filter that takes input composite datasets with the same
// structure: (1) the same number of entries and -- if any children are composites -- the
// same constraint holds from them; and (2) the same type of dataset at each position.
// It then creates an output dataset with the same structure whose leaves contain all the
// cells from the datasets at the corresponding leaves of the input datasets.
//
// Currently, this filter only supports "appending" of a few types for the leaf
// nodes and the logic used for each supported data type is as follows:
//
// \li vtkUnstructuredGrid - appends all unstructured grids from the leaf
//     location on all inputs into a single unstructured grid for the
//     corresponding location in the output composite dataset. PointData and
//     CellData arrays are extracted and appended only if they are available in
//     all datasets.(For example, if one dataset has scalars but another does
//     not, scalars will not be appended.)
//
// \li vtkPolyData - appends all polydatas from the leaf location on all inputs
//     into a single polydata for the corresponding location in the output
//     composite dataset. PointData and CellData arrays are extracted and
//     appended only if they are available in all datasets.(For example, if one
//     dataset has scalars but another does not, scalars will not be appended.)
//
// \li vtkImageData/vtkUniformGrid - simply passes the first non-null
//     grid for a particular location to corresponding location in the output.
//
// \li vtkTable - simply passes the first non-null vtkTable for a particular
//     location to the corresponding location in the output.
//
// Other types of leaf datasets will be ignored and their positions in the
// output dataset will be NULL pointers.
//
// .SECTION See Also
// vtkAppendPolyData vtkAppendFilter

#ifndef __vtkAppendCompositeDataLeaves_h
#define __vtkAppendCompositeDataLeaves_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkCompositeDataSetAlgorithm.h"

class vtkCompositeDataIterator;
class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkAppendCompositeDataLeaves : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkAppendCompositeDataLeaves* New();

  vtkTypeMacro(vtkAppendCompositeDataLeaves,vtkCompositeDataSetAlgorithm);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/get whether the field data of each dataset in the composite dataset is copied to the output.
  // If AppendFieldData is non-zero, then field data arrays from all the inputs are added
  // to the output. If there are duplicates, the array on the first input encountered is taken.
  vtkSetMacro(AppendFieldData,int);
  vtkGetMacro(AppendFieldData,int);
  vtkBooleanMacro(AppendFieldData,int);

protected:
  vtkAppendCompositeDataLeaves();
  ~vtkAppendCompositeDataLeaves();

  // Description:
  // Since vtkCompositeDataSet is an abstract class and we output the same types as the input,
  // we must override the default implementation.
  virtual int RequestDataObject( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  // Description:
  // Iterates over the datasets and appends corresponding notes.
  virtual int RequestData( vtkInformation*, vtkInformationVector**, vtkInformationVector* );

  // Description:
  // The input is repeatable, so we override the default implementation.
  virtual int FillInputPortInformation( int port, vtkInformation* info );

  // Description:
  // When leaf nodes are unstructured grids, this uses a vtkAppendFilter to merge them.
  virtual void AppendUnstructuredGrids(vtkInformationVector* inputVector,
    int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output );

  // Description:
  // When leaf nodes are polydata, this uses a vtkAppendPolyData to merge them.
  virtual void AppendPolyData(vtkInformationVector* inputVector,
    int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output );

  // Description:
  // Both AppendUnstructuredGrids and AppendPolyData call AppendFieldDataArrays. If
  // AppendFieldData is non-zero, then field data arrays from all the inputs are added
  // to the output. If there are duplicates, the array on the first input encountered
  // is taken.
  virtual void AppendFieldDataArrays(vtkInformationVector* inputVector,
    int i, int numInputs, vtkCompositeDataIterator* iter, vtkDataSet* dset );

  int AppendFieldData;

private:
  vtkAppendCompositeDataLeaves ( const vtkAppendCompositeDataLeaves& ); // Not implemented.
  void operator = ( const vtkAppendCompositeDataLeaves& ); // Not implemented.
};

#endif // __vtkAppendCompositeDataLeaves_h
