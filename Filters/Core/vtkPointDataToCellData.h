/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointDataToCellData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointDataToCellData
 * @brief   map point data to cell data
 *
 * vtkPointDataToCellData is a filter that transforms point data (i.e., data
 * specified per point) into cell data (i.e., data specified per cell).
 * The method of transformation is based on averaging the data
 * values of all points defining a particular cell. Optionally, the input point
 * data can be passed through to the output as well.
 *
 * @warning
 * This filter is an abstract filter, that is, the output is an abstract type
 * (i.e., vtkDataSet). Use the convenience methods (e.g.,
 * GetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
 * of output you want.
 *
 * @sa
 * vtkPointData vtkCellData vtkCellDataToPointData
*/

#ifndef vtkPointDataToCellData_h
#define vtkPointDataToCellData_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkPointDataToCellData : public vtkDataSetAlgorithm
{
public:
  static vtkPointDataToCellData *New();
  vtkTypeMacro(vtkPointDataToCellData,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Control whether the input point data is to be passed to the output. If
   * on, then the input point data is passed through to the output; otherwise,
   * only generated point data is placed into the output.
   */
  vtkSetMacro(PassPointData,int);
  vtkGetMacro(PassPointData,int);
  vtkBooleanMacro(PassPointData,int);
  //@}

  //@{
  /**
   * Control whether the input point data is to be treated as categorical. If
   * the data is categorical, then the resultant cell data will be determined by
   * a "majority rules" vote, with ties going to the smaller value.
   */
  vtkSetMacro(CategoricalData,int);
  vtkGetMacro(CategoricalData,int);
  vtkBooleanMacro(CategoricalData,int);
  //@}

protected:
  vtkPointDataToCellData();
  ~vtkPointDataToCellData() VTK_OVERRIDE {}

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) VTK_OVERRIDE;

  int PassPointData;
  int CategoricalData;
private:
  vtkPointDataToCellData(const vtkPointDataToCellData&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointDataToCellData&) VTK_DELETE_FUNCTION;
};

#endif
