// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkComputeQuantiles
 * @brief   Extract Ntiles and extremum values
 * of all columns of a table or all fields of a dataset.
 *
 * vtkComputeQuantiles accepts any vtkDataObject as input and produces a vtkTable data as output
 * containing the extrema and quantiles.
 *
 * The filter internally uses vtkOrderStatistics to divide the dataset into N intervals; so to
 * compute quartiles set the number of intervals to 4, for deciles set the number of intervals to 10
 * etc. The output table has the same number of columns as the input data set and has N+1 number of
 * rows to store the intervals and extrema.
 *
 * @sa
 * vtkTableAlgorithm vtkOrderStatistics
 *
 * @par Thanks:
 * This class was written by Kitware SAS and supported by EDF - www.edf.fr
 */

#ifndef vtkComputeQuantiles_h
#define vtkComputeQuantiles_h

#include "vtkFiltersStatisticsModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkDoubleArray;
class vtkFieldData;
class vtkOrderStatistics;
class vtkTable;

class VTKFILTERSSTATISTICS_EXPORT vtkComputeQuantiles : public vtkTableAlgorithm
{
public:
  static vtkComputeQuantiles* New();
  vtkTypeMacro(vtkComputeQuantiles, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the number of intervals into which the data is to be divided.
   * Default is 4.
   */
  vtkGetMacro(NumberOfIntervals, int);
  vtkSetMacro(NumberOfIntervals, int);
  ///@}

protected:
  vtkComputeQuantiles();
  ~vtkComputeQuantiles() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void ComputeTable(vtkDataObject*, vtkTable*, vtkIdType);

  virtual vtkOrderStatistics* CreateOrderStatisticsFilter();

  int FieldAssociation = -1;
  int NumberOfIntervals = 4;

private:
  void operator=(const vtkComputeQuantiles&) = delete;
  vtkComputeQuantiles(const vtkComputeQuantiles&) = delete;

  int GetInputFieldAssociation();
  vtkFieldData* GetInputFieldData(vtkDataObject* input);
};

VTK_ABI_NAMESPACE_END
#endif
