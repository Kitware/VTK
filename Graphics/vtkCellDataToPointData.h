/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellDataToPointData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellDataToPointData - map cell data to point data
// .SECTION Description
// vtkCellDataToPointData is a filter that transforms cell data (i.e., data
// specified per cell) into point data (i.e., data specified at cell
// points). The method of transformation is based on averaging the data
// values of all cells using a particular point. Optionally, the input cell
// data can be passed through to the output as well.

// .SECTION Caveats
// This filter is an abstract filter, that is, the output is an abstract type
// (i.e., vtkDataSet). Use the convenience methods (e.g.,
// GetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
// of output you want.

// .SECTION See Also
// vtkPointData vtkCellData vtkPointDataToCellData


#ifndef __vtkCellDataToPointData_h
#define __vtkCellDataToPointData_h

#include "vtkDataSetAlgorithm.h"

class vtkDataSet;

class VTK_GRAPHICS_EXPORT vtkCellDataToPointData : public vtkDataSetAlgorithm
{
public:
  static vtkCellDataToPointData *New();
  vtkTypeMacro(vtkCellDataToPointData,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Control whether the input cell data is to be passed to the output. If
  // on, then the input cell data is passed through to the output; otherwise,
  // only generated point data is placed into the output.
  vtkSetMacro(PassCellData,int);
  vtkGetMacro(PassCellData,int);
  vtkBooleanMacro(PassCellData,int);

protected:
  vtkCellDataToPointData();
  ~vtkCellDataToPointData() {};

  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  int PassCellData;
private:
  vtkCellDataToPointData(const vtkCellDataToPointData&);  // Not implemented.
  void operator=(const vtkCellDataToPointData&);  // Not implemented.
};

#endif


