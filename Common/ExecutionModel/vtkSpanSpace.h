// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSpanSpace
 * @brief   organize data according to scalar span space
 *
 * This is a helper class used to accelerate contouring operations. Given an
 * dataset, it organizes the dataset cells into a 2D binned space, with
 * coordinate axes (scalar_min,scalar_max). This so-called span space can
 * then be traversed quickly to find the cells that intersect a specified
 * contour value.
 *
 * This class has an API that supports both serial and parallel
 * operation.  The parallel API enables the using class to grab arrays
 * (or batches) of cells that lie along a particular row in the span
 * space. These arrays can then be processed separately or in parallel.
 *
 * Learn more about span space in these two publications: 1) "A Near
 * Optimal Isosorface Extraction Algorithm Using the Span Space."
 * Yarden Livnat et al. and 2) Isosurfacing in Span Space with Utmost
 * Efficiency." Han-Wei Shen et al.
 *
 * @sa
 * vtkScalarTree vtkSimpleScalarTree
 */

#ifndef vtkSpanSpace_h
#define vtkSpanSpace_h

#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkScalarTree.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkSpanSpace;
struct vtkInternalSpanSpace;

class VTKCOMMONEXECUTIONMODEL_EXPORT vtkSpanSpace : public vtkScalarTree
{
public:
  /**
   * Instantiate a scalar tree with default resolution of 100 and automatic
   * scalar range computation.
   */
  static vtkSpanSpace* New();

  ///@{
  /**
   * Standard type related macros and PrintSelf() method.
   */
  vtkTypeMacro(vtkSpanSpace, vtkScalarTree);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * This method is used to copy data members when cloning an instance of the
   * class. It does not copy heavy data.
   */
  void ShallowCopy(vtkScalarTree* stree) override;

  //----------------------------------------------------------------------
  // The following methods are specific to the creation and configuration of
  // vtkSpanSpace.

  ///@{
  /**
   * Specify the scalar range in terms of minimum and maximum values
   * (smin,smax). These values are used to build the span space. Note that
   * setting the range can have significant impact on the performance of the
   * span space as it controls the effective resolution near important
   * isocontour values. By default the range is computed automatically; turn
   * off ComputeScalarRange is you wish to manually specify it.
   */
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetVectorMacro(ScalarRange, double, 2);
  ///@}

  ///@{
  /**
   * This boolean controls whether the determination of the scalar range is
   * computed from the input scalar data. By default this is enabled.
   */
  vtkSetMacro(ComputeScalarRange, vtkTypeBool);
  vtkGetMacro(ComputeScalarRange, vtkTypeBool);
  vtkBooleanMacro(ComputeScalarRange, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the resolution N of the span space. The span space can be
   * envisioned as a rectangular lattice of NXN buckets/bins (i.e., N rows
   * and N columns), where each bucket stores a list of cell ids. The i-j
   * coordinate of each cell (hence its location in the lattice) is
   * determined from the cell's 2-tuple (smin,smax) scalar range.  By default
   * Resolution = 100, with a clamp of 10,000.
   */
  vtkSetClampMacro(Resolution, vtkIdType, 1, 10000);
  vtkGetMacro(Resolution, vtkIdType);
  ///@}

  ///@{
  /**
   * Boolean controls whether the resolution of span space is computed
   * automatically from the average number of cells falling in each bucket.
   */
  vtkSetMacro(ComputeResolution, vtkTypeBool);
  vtkGetMacro(ComputeResolution, vtkTypeBool);
  vtkBooleanMacro(ComputeResolution, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the average number of cells in each bucket. This is used to
   * indirectly control the resolution if ComputeResolution is enabled.
   */
  vtkSetClampMacro(NumberOfCellsPerBucket, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfCellsPerBucket, int);
  ///@}

  //----------------------------------------------------------------------
  // The following methods satisfy the vtkScalarTree abstract API.

  /**
   * Initialize the span space. Frees memory and resets object as
   * appropriate.
   */
  void Initialize() override;

  /**
   * Construct the scalar tree from the dataset provided. Checks build times
   * and modified time from input and reconstructs the tree if necessary.
   */
  void BuildTree() override;

  /**
   * Begin to traverse the cells based on a scalar value. Returned cells will
   * have scalar values that span the scalar value specified (within the
   * resolution of the span space). Note this method must be called prior to
   * parallel or serial traversal since it specifies the scalar value to be
   * extracted.
   */
  void InitTraversal(double scalarValue) override;

  /**
   * Return the next cell that may contain scalar value specified to
   * InitTraversal(). The value nullptr is returned if the list is
   * exhausted. Make sure that InitTraversal() has been invoked first or
   * you'll get undefined behavior. This is inherently a serial operation.
   */
  vtkCell* GetNextCell(vtkIdType& cellId, vtkIdList*& ptIds, vtkDataArray* cellScalars) override;

  // The following methods supports parallel (threaded) traversal. Basically
  // batches of cells (which are a portion of the whole dataset) are available for
  // processing in a parallel For() operation.

  /**
   * Get the number of cell batches available for processing as a function of
   * the specified scalar value. Each batch contains a list of candidate
   * cells that may contain the specified isocontour value.
   */
  vtkIdType GetNumberOfCellBatches(double scalarValue) override;

  /**
   * Return the array of cell ids in the specified batch. The method
   * also returns the number of cell ids in the array. Make sure to
   * call GetNumberOfCellBatches() beforehand.
   */
  const vtkIdType* GetCellBatch(vtkIdType batchNum, vtkIdType& numCells) override;

  ///@{
  /**
   * Set/Get the size of the cell batches when processing in
   * parallel. By default the batch size = 100 cells in each batch.
   */
  vtkSetClampMacro(BatchSize, vtkIdType, 100, VTK_INT_MAX);
  vtkGetMacro(BatchSize, vtkIdType);
  ///@}

protected:
  vtkSpanSpace();
  ~vtkSpanSpace() override;

  double ScalarRange[2];
  vtkTypeBool ComputeScalarRange;
  vtkIdType Resolution;
  vtkTypeBool ComputeResolution;
  int NumberOfCellsPerBucket;
  vtkInternalSpanSpace* SpanSpace;
  vtkIdType BatchSize;

private:
  // Internal variables supporting span space traversal
  vtkIdType RMin[2]; // span space lower left corner
  vtkIdType RMax[2]; // span space upper right corner

  // This supports serial traversal via GetNextCell()
  vtkIdType CurrentRow;      // the span space row currently being processed
  vtkIdType* CurrentSpan;    // pointer to current span row
  vtkIdType CurrentIdx;      // position into the current span row
  vtkIdType CurrentNumCells; // number of cells on the current span row

  vtkSpanSpace(const vtkSpanSpace&) = delete;
  void operator=(const vtkSpanSpace&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
