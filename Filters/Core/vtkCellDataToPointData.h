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
 * Unstructured grids and polydata can have cells of different dimensions.
 * To handle different use cases in this situation, the user can specify
 * which cells contribute to the computation. The options for this are
 * All (default), Patch and DataSetMax. Patch uses only the highest dimension
 * cells attached to a point. DataSetMax uses the highest cell dimension in
 * the entire data set.
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Options to choose what cells contribute to the calculation
  enum ContributingCellEnum {
    All=0,        //!< All cells
    Patch=1,      //!< Highest dimension cells in the patch of cells contributing to the calculation
    DataSetMax=2  //!< Highest dimension cells in the data set
  };

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

  //@{
  /**
   * Option to specify what cells to include in the gradient computation.
   * Options are all cells (All, Patch and DataSetMax). The default is All.
   */
  vtkSetClampMacro(ContributingCellOption, int, 0, 2);
  vtkGetMacro(ContributingCellOption, int);
  //@}

protected:
  vtkCellDataToPointData();
  ~vtkCellDataToPointData() override {}

  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  //@{
  /**
   * Special algorithm for unstructured grids and polydata to make sure
   * that we properly take into account ContributingCellOption.
   */
  int RequestDataForUnstructuredData
    (vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  //@}

  void InterpolatePointData(vtkDataSet *input, vtkDataSet *output);

  //@{
  /**
   * Option to pass cell data arrays through to the output. Default is 0/off.
   */
  int PassCellData;
  //@}

  //@{
  /**
   * Option to specify what cells to include in the computation.
   * Options are all cells (All, Patch and DataSet). The default is All.
   */
  int ContributingCellOption;
  //@}

private:
  vtkCellDataToPointData(const vtkCellDataToPointData&) = delete;
  void operator=(const vtkCellDataToPointData&) = delete;
};

#endif


