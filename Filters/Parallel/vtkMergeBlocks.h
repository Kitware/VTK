// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkMergeBlocks
 * @brief merges blocks in a composite dataset to a dataset.
 *
 * vtkMergeBlocks merges all blocks in a composite-dataset (rather any
 * vtkDataObjectTree subclass) into a dataset (either vtkPolyData or
 * vtkUnstructuredGrid based on `vtkMergeBlocks::OutputDataSetType`).
 *
 * If vtkMergeBlocks::MergePartitionsOnly is true, then only
 * vtkPartitionedDataSet (and vtkMultiPieceDataSet) blocks will be merged, thus
 * largely preserving the tree structure.
 *
 * @sa vtkGroupDataSetsFilter
 */

#ifndef vtkMergeBlocks_h
#define vtkMergeBlocks_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSPARALLEL_EXPORT vtkMergeBlocks : public vtkDataObjectAlgorithm
{
public:
  static vtkMergeBlocks* New();
  vtkTypeMacro(vtkMergeBlocks, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Turn on/off merging of coincidental points.  Frontend to
   * vtkAppendFilter::MergePoints. Default is on.
   */
  vtkSetMacro(MergePoints, bool);
  vtkGetMacro(MergePoints, bool);
  vtkBooleanMacro(MergePoints, bool);
  ///@}

  ///@{
  /**
   * Get/Set the tolerance to use to find coincident points when `MergePoints`
   * is `true`. Default is 0.0.
   *
   * This is simply passed on to the internal vtkAppendFilter::vtkLocator used to merge points.
   * @sa `vtkLocator::SetTolerance`.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Get/Set whether Tolerance is treated as an absolute or relative tolerance.
   * The default is to treat it as an absolute tolerance.
   */
  vtkSetMacro(ToleranceIsAbsolute, bool);
  vtkGetMacro(ToleranceIsAbsolute, bool);
  vtkBooleanMacro(ToleranceIsAbsolute, bool);
  ///@}

  ///@{
  /**
   * When set to true, only vtkPartitionedDataSet and vtkMultiPieceDataSet
   * instances are merged into a since vtkUnstructuredGrid leaving parent
   * vtkMultiBlockDataSet or vtkPartitionedDataSetCollection structure largely
   * unchanged.
   */
  vtkSetMacro(MergePartitionsOnly, bool);
  vtkGetMacro(MergePartitionsOnly, bool);
  vtkBooleanMacro(MergePartitionsOnly, bool);
  ///@}

  ///@{
  /**
   * Get/Set the output type produced by this filter. Only blocks compatible with the output type
   * will be merged in the output. For example, if the output type is vtkPolyData, then
   * blocks of type vtkImageData, vtkStructuredGrid, etc. will not be merged - only vtkPolyData
   * can be merged into a vtkPolyData. On the other hand, if the output type is
   * vtkUnstructuredGrid, then blocks of almost any type will be merged in the output.
   * Valid values are VTK_POLY_DATA and VTK_UNSTRUCTURED_GRID defined in vtkType.h.
   * Defaults to VTK_UNSTRUCTURED_GRID.
   */
  vtkSetMacro(OutputDataSetType, int);
  vtkGetMacro(OutputDataSetType, int);
  ///@}

protected:
  vtkMergeBlocks();
  ~vtkMergeBlocks() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool MergePoints;
  double Tolerance;
  bool MergePartitionsOnly;
  bool ToleranceIsAbsolute;
  int OutputDataSetType;

private:
  vtkMergeBlocks(const vtkMergeBlocks&) = delete;
  void operator=(const vtkMergeBlocks&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
