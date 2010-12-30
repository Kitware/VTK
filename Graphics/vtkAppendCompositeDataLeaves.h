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
// Currently, only input polydata and unstructured grids are handled; other types of
// leaf datasets will be ignored and their positions in the output dataset will be NULL pointers.
// Point attributes (i.e., scalars, vectors, normals, field data, etc.) are extracted 
// and appended only if all datasets have the point attributes available. 
// (For example, if one dataset has scalars but another does not, scalars will 
// not be appended.)
//
// .SECTION See Also
// vtkAppendPolyData vtkAppendFilter

#ifndef __vtkAppendCompositeDataLeaves_h
#define __vtkAppendCompositeDataLeaves_h

#include "vtkCompositeDataSetAlgorithm.h"

class vtkAppendFilter;
class vtkAppendPolyData;
class vtkCompositeDataIterator;
class vtkDataSet;
class vtkPolyData;
class vtkUnstructuredGrid;

class VTK_GRAPHICS_EXPORT vtkAppendCompositeDataLeaves : public vtkCompositeDataSetAlgorithm
{
public:
  static vtkAppendCompositeDataLeaves* New();

  vtkTypeMacro(vtkAppendCompositeDataLeaves,vtkCompositeDataSetAlgorithm);
  void PrintSelf( ostream& os, vtkIndent indent );

//BTX
  // Description:
  // Get any input of this filter.
  vtkCompositeDataSet* GetInput( int idx );
  vtkCompositeDataSet* GetInput() 
    { return this->GetInput( 0 ); }
//ETX

  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput( vtkDataSet* in );

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
  virtual void AppendUnstructuredGrids( int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output );

  // Description:
  // When leaf nodes are polydata, this uses a vtkAppendPolyData to merge them.
  virtual void AppendPolyData( int i, int numInputs, vtkCompositeDataIterator* iter, vtkCompositeDataSet* output );

  // Description:
  // Both AppendUnstructuredGrids and AppendPolyData call AppendFieldDataArrays. If
  // AppendFieldData is non-zero, then field data arrays from all the inputs are added
  // to the output. If there are duplicates, the array on the first input encountered
  // is taken.
  virtual void AppendFieldDataArrays( int i, int numInputs, vtkCompositeDataIterator* iter, vtkDataSet* dset );

  int AppendFieldData;
  vtkAppendFilter* AppendUG;
  vtkAppendPolyData* AppendPD;

private:
  vtkAppendCompositeDataLeaves ( const vtkAppendCompositeDataLeaves& ); // Not implemented.
  void operator = ( const vtkAppendCompositeDataLeaves& ); // Not implemented.
};

#endif // __vtkAppendCompositeDataLeaves_h
