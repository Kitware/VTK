/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAppendPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#ifndef vtkAppendPolyData_h
#define vtkAppendPolyData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkDataArray;
class vtkPoints;
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtkAppendPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkAppendPolyData *New();

  vtkTypeMacro(vtkAppendPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // UserManagedInputs allows the user to set inputs by number instead of
  // using the AddInput/RemoveInput functions. Calls to
  // SetNumberOfInputs/SetInputConnectionByNumber should not be mixed with calls
  // to AddInput/RemoveInput. By default, UserManagedInputs is false.
  vtkSetMacro(UserManagedInputs,int);
  vtkGetMacro(UserManagedInputs,int);
  vtkBooleanMacro(UserManagedInputs,int);

  // Description:
  // Add a dataset to the list of data to append. Should not be
  // used when UserManagedInputs is true, use SetInputByNumber instead.
  void AddInputData(vtkPolyData *);

  // Description:
  // Remove a dataset from the list of data to append. Should not be
  // used when UserManagedInputs is true, use SetInputByNumber (NULL) instead.
  void RemoveInputData(vtkPolyData *);

//BTX
  // Description:
  // Get any input of this filter.
  vtkPolyData *GetInput(int idx);
  vtkPolyData *GetInput() { return this->GetInput( 0 ); };
//ETX

  // Description:
  // Directly set(allocate) number of inputs, should only be used
  // when UserManagedInputs is true.
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputConnectionByNumber(int num, vtkAlgorithmOutput *input);
  void SetInputDataByNumber(int num, vtkPolyData *ds);

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

  // Description:
  // Set/get the desired precision for the output types. See the documentation
  // for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
  // the available precision settings.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

//BTX
  int ExecuteAppend(vtkPolyData* output,
    vtkPolyData* inputs[], int numInputs);
//ETX
protected:
  vtkAppendPolyData();
  ~vtkAppendPolyData();

  // Flag for selecting parallel streaming behavior
  int ParallelStreaming;
  int OutputPointsPrecision;

  // Usual data generation method
  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *,
                                  vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);

  // An efficient way to append data/cells.
  void AppendData(vtkDataArray *dest, vtkDataArray *src, vtkIdType offset);
  void AppendDifferentPoints(vtkDataArray *dest, vtkDataArray *src,
                             vtkIdType offset);
  vtkIdType *AppendCells(vtkIdType *pDest, vtkCellArray *src,
                         vtkIdType offset);

 private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInputData(vtkDataObject *)
    { vtkErrorMacro( << "AddInput() must be called with a vtkPolyData not a vtkDataObject."); };

  int UserManagedInputs;

private:
  vtkAppendPolyData(const vtkAppendPolyData&);  // Not implemented.
  void operator=(const vtkAppendPolyData&);  // Not implemented.
};

#endif


