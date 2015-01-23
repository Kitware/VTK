/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#ifndef vtkAppendFilter_h
#define vtkAppendFilter_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkDataSetAttributes;
class vtkDataSetCollection;

class VTKFILTERSCORE_EXPORT vtkAppendFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkAppendFilter *New();

  vtkTypeMacro(vtkAppendFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Get any input of this filter.
  vtkDataSet *GetInput(int idx);
  vtkDataSet *GetInput()
    {return this->GetInput( 0 );}
//ETX

  // Description:
  // Get if the filter should merge coincidental points
  // Note: The filter will only merge points if the ghost cell array doesn't exist
  // Defaults to Off
  vtkGetMacro(MergePoints,int);

  // Description:
  // Set the filter to merge coincidental points.
  // Note: The filter will only merge points if the ghost cell array doesn't exist
  // Defaults to Off
  vtkSetMacro(MergePoints,int);

  vtkBooleanMacro(MergePoints,int);

  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInputData(vtkDataSet *in);

  // Description:
  // Returns a copy of the input array.  Modifications to this list
  // will not be reflected in the actual inputs.
  vtkDataSetCollection *GetInputList();

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::Precision enum for an explanation of the available
  // precision settings.
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);

protected:
  vtkAppendFilter();
  ~vtkAppendFilter();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // list of data sets to append together.
  // Here as a convenience.  It is a copy of the input array.
  vtkDataSetCollection *InputList;

  //If true we will attempt to merge points. Must also not have
  //ghost cells defined.
  int MergePoints;

  int OutputPointsPrecision;

private:
  vtkAppendFilter(const vtkAppendFilter&);  // Not implemented.
  void operator=(const vtkAppendFilter&);  // Not implemented.

  // Get all input data sets that have points, cells, or both.
  // Caller must delete the returned vtkDataSetCollection.
  vtkDataSetCollection* GetNonEmptyInputs(vtkInformationVector ** inputVector);

  void AppendArrays(int attributesType,
                    vtkInformationVector **inputVector,
                    vtkIdType* globalIds,
                    vtkUnstructuredGrid* output);
};


#endif


