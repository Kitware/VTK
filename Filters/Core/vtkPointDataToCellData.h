// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointDataToCellData
 * @brief   map point data to cell data
 *
 * vtkPointDataToCellData is a filter that transforms point data (i.e., data
 * specified per point) into cell data (i.e., data specified per cell).  By
 * default, the method of transformation is based on averaging the data
 * values of all the points defining a particular cell. Optionally (by enabling
 * CategoricalData), histograming can be used to assign the cell data. For
 * large datasets with several cell data arrays, the filter optionally
 * supports selective processing to speed up processing. Optionally, the
 * input point data can be passed through to the output as well.
 *
 * @warning
 * This filter is an abstract filter, that is, the output is an abstract type
 * (i.e., vtkDataSet). Use the convenience methods (e.g.,
 * GetPolyDataOutput(), GetStructuredPointsOutput(), etc.) to get the type
 * of output you want.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkPointData vtkCellData vtkCellDataToPointData
 */

#ifndef vtkPointDataToCellData_h
#define vtkPointDataToCellData_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkPointDataToCellData : public vtkDataSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkPointDataToCellData* New();
  vtkTypeMacro(vtkPointDataToCellData, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Control whether the input point data is to be passed to the output. If
   * on, then the input point data is passed through to the output; otherwise,
   * only generated cell data is placed into the output.
   */
  vtkSetMacro(PassPointData, bool);
  vtkGetMacro(PassPointData, bool);
  vtkBooleanMacro(PassPointData, bool);
  ///@}

  ///@{
  /**
   * Control whether the input point data is to be treated as categorical. If
   * the data is categorical, then the resultant cell data will be determined
   * by a "majority rules" vote (using a histogram of the point data scalar
   * values), with ties going to the smaller point data value.
   */
  vtkSetMacro(CategoricalData, bool);
  vtkGetMacro(CategoricalData, bool);
  vtkBooleanMacro(CategoricalData, bool);
  ///@}

  ///@{
  /**
   * Activate selective processing of arrays. If inactive, only arrays selected
   * by the user will be considered by this filter. The default is true.
   */
  vtkSetMacro(ProcessAllArrays, bool);
  vtkGetMacro(ProcessAllArrays, bool);
  vtkBooleanMacro(ProcessAllArrays, bool);
  ///@}

  /**
   * Adds an array to be processed. This only has an effect if the
   * ProcessAllArrays option is turned off. If a name is already present,
   * nothing happens.
   */
  virtual void AddPointDataArray(const char* name);

  /**
   * Removes an array to be processed. This only has an effect if the
   * ProcessAllArrays option is turned off. If the specified name is not
   * present, nothing happens.
   */
  virtual void RemovePointDataArray(const char* name);

  /**
   * Removes all arrays to be processed from the list. This only has an effect
   * if the ProcessAllArrays option is turned off.
   */
  virtual void ClearPointDataArrays();

protected:
  vtkPointDataToCellData();
  ~vtkPointDataToCellData() override;

  virtual vtkIdType GetNumberOfPointArraysToProcess();
  virtual void GetPointArraysToProcess(const char* names[]);

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  bool PassPointData;
  bool CategoricalData;
  bool ProcessAllArrays;

  class Internals;
  Internals* Implementation;

private:
  vtkPointDataToCellData(const vtkPointDataToCellData&) = delete;
  void operator=(const vtkPointDataToCellData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
