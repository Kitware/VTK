// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExplodeDataSet
 * @brief   Explode input dataset according to a cell scalar isovalue criteria.
 *
 * vtkExplodeDataSet creates a vtkPartitionedDataSetCollection containing
 * isovalue partitions: each partition of the output contains cells that share
 * the same value for the given cell array.
 *
 * Output is a vtkPartitionedDataSetCollection where leaves are vtkUnstructuredGrid,
 * unless input is a vtkPolyData, then leaves stay vtkPolyData.
 *
 * PointData and CellData are forwarded accordingly.
 *
 * It is similar to a vtkMultiThreshold, configured to extract every single
 * value of the input array.
 *
 * It comes in replacement of vtkSplitByCellScalarFilter to use
 * vtkPartitionedDataSetCollection instead of vtkMultiBlockDataSet as output.
 * Also, vtkExplodeDataSet relies internally on vtkExtractCells to generate
 * output datasets. This benefits from SMPTools threading acceleration.
 *
 * @sa vtkExtractCells, vtkMultiThreshold
 */

#ifndef vtkExplodeDataSet_h
#define vtkExplodeDataSet_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkIdList;
class vtkPointSet;

class VTKFILTERSGENERAL_EXPORT vtkExplodeDataSet : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkExplodeDataSet* New();
  vtkTypeMacro(vtkExplodeDataSet, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExplodeDataSet();
  ~vtkExplodeDataSet() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Accepts vtkDataSet as input.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkExplodeDataSet(const vtkExplodeDataSet&) = delete;
  void operator=(const vtkExplodeDataSet&) = delete;

  /**
   * Fill the idsByValue map: for each value in scalars, list all of input cells that
   * matches this value.
   */
  std::map<double, vtkSmartPointer<vtkIdList>> GetIdListsByValue(vtkDataArray* scalars);

  /**
   * Create a partition from input cells and given points.
   * If input is a vtkPolyData, returns a vtkPolyData. Otherwise returns a vtkUnstructuredGrid.
   * Caller is responsible for deletion.
   */
  vtkPointSet* CreatePartition(vtkDataSet* input, vtkIdList* partCellIds);
};

VTK_ABI_NAMESPACE_END
#endif
