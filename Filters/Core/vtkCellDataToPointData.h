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
/**
 * @class   vtkCellDataToPointData
 * @brief   map cell data to point data
 *
 * vtkCellDataToPointData is a filter that transforms cell data (i.e., data
 * specified per cell) into point data (i.e., data specified at cell
 * points). The method of transformation is based on averaging the data
 * values of all cells using a particular point. Optionally, the input cell
 * data can be passed through to the output as well.
 *
 * @warning
 * This filter is an abstract filter, that is, the output is an abstract type
 * (i.e., vtkDataSet). Use the convenience methods (e.g.,
 * GetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
 * of output you want.
 *
 * @sa
 * vtkPointData vtkCellData vtkPointDataToCellData
*/

#ifndef vtkCellDataToPointData_h
#define vtkCellDataToPointData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkDataSet;

class VTKFILTERSCORE_EXPORT vtkCellDataToPointData : public vtkDataSetAlgorithm
{
public:
  static vtkCellDataToPointData *New();
  vtkTypeMacro(vtkCellDataToPointData,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Control whether the input cell data is to be passed to the output. If
   * on, then the input cell data is passed through to the output; otherwise,
   * only generated point data is placed into the output.
   */
  vtkSetMacro(PassCellData,int);
  vtkGetMacro(PassCellData,int);
  vtkBooleanMacro(PassCellData,int);
  //@}

protected:
  vtkCellDataToPointData();
  ~vtkCellDataToPointData() VTK_OVERRIDE {}

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) VTK_OVERRIDE;

  // Special traversal algorithm for unstructured grid
  int RequestDataForUnstructuredGrid
    (vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  void interpolatePointData(vtkDataSet *input, vtkDataSet *output);

  // Same as above, but with special handling for masked cells in input.
  void interpolatePointDataWithMask(vtkStructuredGrid *input,
                                    vtkDataSet *output);

  int PassCellData;
private:
  vtkCellDataToPointData(const vtkCellDataToPointData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellDataToPointData&) VTK_DELETE_FUNCTION;
};

#endif


